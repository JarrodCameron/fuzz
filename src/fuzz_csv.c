#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "csv.h"
#include "fuzz_csv.h"
#include "fuzzer.h"
#include "safe.h"
#include "utils.h"

struct value {
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

	return;
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
