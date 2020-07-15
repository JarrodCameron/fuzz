#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "csv.h"
#include "fuzz_csv.h"
#include "fuzzer.h"
#include "safe.h"
#include "utils.h"

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

/* Here we add functions used for fuzzing.
 * Each function tests for something different. */
static void (*fuzz_payloads[])(struct state *) = {
	fuzz_buffer_overflow,
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
	/* XXX We might want a loop here? */
	fuzz_payloads[0](s);
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
