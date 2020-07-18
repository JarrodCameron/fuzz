#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "csv.h"
#include "fuzz_csv.h"
#include "fuzzer.h"
#include "safe.h"
#include "utils.h"
#include "mutation_functions.h"

struct value {
	uint64_t len;
	char *val;
};

/* Row in a csv, may contain multiple '\n' */
struct row {
	/* The entire row in plain text */
	char *row;

	/* Number of values in this row */
	uint64_t nvals;

	/* Array of values in the given row */
	struct value *vals;
};

static struct {

	/* Number of rows in csv file */
	uint64_t nrows;

	struct row *rows;

} csv = {0};

/* Helper functions */
static uint64_t arr_len(char **arr);
static void handle_row(struct row *row, char *text);
static void fuzz_buffer_overflow(struct state *s);
static void try_buffer_overflow(uint64_t row, uint64_t val, struct state *s);
static void fuzz(struct state *s);
static void try_bad_nums(uint64_t row, uint64_t val, struct state *s);
static void fuzz_bad_nums(struct state *s);
static void dump_csv(struct state *s);
static void fuzz_populate(struct state *s);
static off_t dump_row(int fd, struct row *row);

/* Here we add functions used for fuzzing.
 * Each function tests for something different. */
static void (*fuzz_payloads[])(struct state *) = {
	fuzz_buffer_overflow,
	fuzz_bad_nums,
	fuzz_populate,
};

void
fuzz_handle_csv(struct state *state)
{
	char **rows = split_on_unescaped_newlines(state->mem, state->stat.st_size);
	if(!rows)
		panic("Failed to init csv parser\n");

	csv.nrows = arr_len(rows);
	csv.rows = smalloc(sizeof(struct row) * csv.nrows);

	/* Now we split all the values in a given row of the csv */
	for (uint64_t i = 0; i < csv.nrows; i++)
		handle_row(&csv.rows[i], rows[i]);

	free(rows);

	fuzz(state);
}

/* Does the actual fuzzing */
static
void
fuzz(struct state *s)
{
	//Example of a call
	bit_shift_in_range(s->payload_fd, 0 ,0);
	while (1) {
		uint32_t idx = roll_dice(0, ARRSIZE(fuzz_payloads)-1);
		fuzz_payloads[idx](s);
	}
}

/* For each row in the csv, flip a bias coin to see if we should
 * print it or not */
static
void
fuzz_populate(struct state *s)
{
	uint64_t i, len = 0;

	slseek(s->payload_fd, 0, SEEK_SET);

	for (i = 0; i < csv.nrows; i++) {
		if (coin_flip(90)) {
			len += dump_row(s->payload_fd, &(csv.rows[i]));
			i--;
		}
	}
	sftruncate(s->payload_fd, len);
	deploy();
}

/* Print the row to the file, simple */
static
off_t
dump_row(int fd, struct row *row)
{
	off_t len = 0;

	for (uint64_t v = 0; v < row->nvals; v++) {

		/* Write the value in the csv */
		len += swrite(
			fd,
			row->vals[v].val,
			row->vals[v].len
		);

		/* Seperate each element by a "," unless it is the last element in
		 * a row, then seperate with a "\n" */
		if (v == row->nvals - 1)
			len += swrite(fd, "\n", 1);
		else
			len += swrite(fd, ",", 1);
	}

	return len;
}

/* Here we look for numbers in our csv file and change then to crazy values */
static
void
fuzz_bad_nums(struct state *s)
{
	uint64_t row, val;
	for (row = 0; row < csv.nrows; row++) {
		for (val = 0; val < csv.rows[row].nvals; val++) {
			try_bad_nums(row, val, s);
		}
	}
}

static
void
fuzz_buffer_overflow(struct state *s)
{
	uint64_t row, val;
	for (row = 0; row < csv.nrows; row++) {
		for (val = 0; val < csv.rows[row].nvals; val++) {
			try_buffer_overflow(row, val, s);
		}
	}
}

static
void
try_bad_nums(uint64_t row, uint64_t val, struct state *s)
{
	char *old_val = csv.rows[row].vals[val].val;
	uint64_t old_len = csv.rows[row].vals[val].len;

	for (uint64_t i = 0; i < ARRSIZE(bad_nums); i++) {
		csv.rows[row].vals[val].val = (char *) bad_nums[i].s;
		csv.rows[row].vals[val].len = strlen(bad_nums[i].s);
		dump_csv(s);
		deploy();
	}

	csv.rows[row].vals[val].val = old_val;
	csv.rows[row].vals[val].len = old_len;

}

/* Write the csv file to state->payload_fd */
static
void
dump_csv(struct state *s)
{
	off_t len = 0;
	uint64_t r, v;

	slseek(s->payload_fd, 0, SEEK_SET);

	for (r = 0; r < csv.nrows; r++) { /* For each row in csv... */
		for (v = 0; v < csv.rows[r].nvals; v++) { /* For each value in row... */

			/* Write the value in the csv */
			len += swrite(
				s->payload_fd,
				csv.rows[r].vals[v].val,
				csv.rows[r].vals[v].len
			);

			/* Seperate each element by a "," unless it is the last element in
			 * a row, then seperate with a "\n" */
			if (v == csv.rows[r].nvals - 1)
				len += swrite(s->payload_fd, "\n", 1);
			else
				len += swrite(s->payload_fd, ",", 1);
		}
	}
	sftruncate(s->payload_fd, len);
}

static
void
try_buffer_overflow(uint64_t row, uint64_t val, struct state *s)
{
	slseek(s->payload_fd, 0, SEEK_SET);

	off_t len = 0;
	uint64_t r, v;
	for (r = 0; r < csv.nrows; r++) {
		for (v = 0; v < csv.rows[r].nvals; v++) {

			/* Insert the csv value into the payload
			 * One value will contain a massive string */
			if (r == row && v == val) {
				len += swrite(s->payload_fd, BIG, sizeof(BIG)-1);
			} else {
				len += swrite(
					s->payload_fd,
					csv.rows[r].vals[v].val,
					csv.rows[r].vals[v].len
				);
			}

			if (v == csv.rows[r].nvals - 1)
				len += swrite(s->payload_fd, "\n", 1);
			else
				len += swrite(s->payload_fd, ",", 1);
		}
	}

	sftruncate(s->payload_fd, len);
	deploy();
}

/* Splits one row of the csv file */
static
void
handle_row(struct row *row, char *text)
{
	char **arr = parse_csv(text);
	if (!arr)
		panic("Failed to split csv line: \"%s\"\n", text);

	row->row = text;
	row->nvals = arr_len(arr);
	row->vals = smalloc(sizeof(struct value) * csv.nrows);

	for (uint64_t i = 0; i < row->nvals; i++) {
		row->vals[i].val = arr[i];
		row->vals[i].len = strlen(arr[i]);
	}

	free(arr);
}

/* Return the number of items in the array. This function assumes the last
 * element in the array is NULL. */
static
uint64_t
arr_len(char **arr)
{
	uint64_t i = 0;
	while(arr[i])
		i++;
	return i;
}
