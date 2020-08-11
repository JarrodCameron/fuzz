#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "csv.h"
#include "fs.h"
#include "fuzz_csv.h"
#include "fuzzer.h"
#include "mutation_functions.h"
#include "safe.h"
#include "utils.h"

struct value {
	uint64_t len;
	uint64_t orig_len;
	char *val;
	char *orig_val;

	/* Where this value has been artifically added
	and should be removed when datasctructure is reverted */
	uint64_t added_val;

	struct value *next;
	struct value *previous;

};

/* Row in a csv, may contain multiple '\n' */
struct row {
	/* The entire row in plain text */
	char *row;
	char *orig_row;

	/* Number of values in this row */
	uint64_t nvals;
	uint64_t orig_nvals;

	/* Where this row has been artifically added
	and should be removed when data sctructure is reverted */
	uint64_t added_row;

	/* pointer to linked list of values */
	struct value *vals;

	/* pointer to next row in LL */
	struct row *next;
	struct row * previous;

};

static struct {

	/* Number of rows in csv file */
	uint64_t nrows;
	uint64_t orig_nrows;


	/* Linked list of rows */
	struct row *rows;

} csv = {0};

/* Helper functions */
static void handle_row(struct row *row, char *text);
static void fuzz_buffer_overflow(struct state *s);
static void try_buffer_overflow(struct value * value_to_fuzz, struct state *s);
static void fuzz(struct state *s);
static void try_bad_nums(struct value * value_to_fuzz, struct state *s);
static void fuzz_bad_nums(struct state *s);
static off_t dump_csv(struct state *s);
static void fuzz_populate_length(struct state *s);
static off_t dump_row(int fd, struct row *row);
static void revert_csv_data_structures(void);
static void insert_value(uint32_t row, uint32_t col, char* value);
static void try_special_chars(struct value* value_to_fuzz, struct state *s);
static void fuzz_special_chars(struct state *s);
static void fuzz_populate_width(struct state *s);
static void fuzz_empty_cells(struct state *s);
static void fuzz_bit_shift_flip_control_char(struct state *s);
static void fuzz_bit_shift_flip_chars(struct state *s);


/* Here are the functions that can be run multiple times and
   do different things each time. */
static void (*fuzz_payloads_repeat[])(struct state *) = {
	fuzz_populate_width,
	fuzz_populate_length,
	fuzz_empty_cells,
	fuzz_bit_shift_flip_chars,
};

/* Here are the functions that should be only run once */
static void (*fuzz_payloads_single[])(struct state *) = {
	fuzz_special_chars,
	fuzz_bad_nums,
	fuzz_buffer_overflow,
	fuzz_bit_shift_flip_control_char,
};



void
fuzz_handle_csv(struct state *state)
{
	char **rows = split_on_unescaped_newlines(state->mem, state->stat.st_size);
	if(!rows)
		panic("Failed to init csv parser\n");

	csv.nrows = arr_len((const void **)rows);
	csv.orig_nrows = csv.nrows;
	csv.rows = smalloc(sizeof(struct row)); //malloc the first LL item

	/* Now we split all the values in a given row of the csv */
	struct row* curr_row = csv.rows;
	struct row* prev_row = NULL;
	for (uint64_t i = 0; i < csv.nrows; i++) {
		handle_row(curr_row, rows[i]);
		curr_row->next = smalloc(sizeof(struct row));
		curr_row->previous = prev_row;
		prev_row = curr_row;
		curr_row = curr_row->next;
	}

	//We have added an extra one at the end that is not needed
	prev_row->next = NULL;
	free(curr_row);

	free(rows);

	fuzz(state);
}

/* Does the actual fuzzing */
static
void
fuzz(struct state *s)
{

	uint64_t i = 0;
	while(i < ARRSIZE(fuzz_payloads_single)) {
		fuzz_payloads_single[i](s);
		i++;
	}

	while (1) {
		uint32_t idx = roll_dice(0, ARRSIZE(fuzz_payloads_repeat)-1);
		fuzz_payloads_repeat[idx](s);
	}
}



static
void
revert_values_data_structure(struct row * row_to_revert) {
	struct value * curr_val = row_to_revert->vals;
	struct value * prev_val = NULL;
	while (curr_val != NULL) {
		if(curr_val->added_val == TRUE) {
			//Don't include value when reverting data strcuture
			if (curr_val->orig_val != NULL) free(curr_val->orig_val);
			if (curr_val->val != NULL) free(curr_val->val);
			//E.g. if it is the head of the list then fix up the head of the list
			if (curr_val == row_to_revert->vals) {
				row_to_revert->vals = curr_val->next;
				struct value * temp = curr_val;
				if(curr_val->next == NULL) {
					//This is the only value in the list
					row_to_revert->vals = NULL;
					curr_val = NULL;
				}
				else {
					curr_val = curr_val->next;
					curr_val->previous = NULL;
					prev_val = NULL;
				}
				free(temp);
			} //Current value is at the end of the list
			else if (curr_val->next == NULL) {
				prev_val->next = NULL;
				free(curr_val);
				curr_val = NULL;
			}
			else {
				prev_val->next = curr_val->next;
				struct value * temp = curr_val;
				curr_val = curr_val->next;
				curr_val->previous = prev_val;
				free(temp);
			}
		}
		else {

			curr_val->len = curr_val->orig_len;
			if (curr_val->val != NULL) free(curr_val->val);
			curr_val->val = strdup(curr_val->orig_val);
			prev_val = curr_val;
			curr_val = curr_val->next;

		}
	}
}


/* reverts the global var csv to its original form */
static
void
revert_csv_data_structures(void)
{
	csv.nrows = csv.orig_nrows;

	struct row * curr_row = csv.rows;
	struct row * prev_row = NULL;

	while(curr_row != NULL) {
		if (curr_row->added_row == TRUE) {
			//Don't include this row when reverting structure

			if (curr_row->row != NULL) free(curr_row->row);
			if (curr_row->orig_row != NULL) free(curr_row->orig_row);
			revert_values_data_structure(curr_row);

			struct row * temp = curr_row;
			curr_row = curr_row->next;
			free(temp);


		}
		else {
			if (curr_row->row != NULL) free(curr_row->row);
			curr_row->row = strdup(curr_row->orig_row);
			curr_row->nvals = curr_row->orig_nvals;
			revert_values_data_structure(curr_row);
			prev_row = curr_row;
			curr_row = curr_row->next;


		}

	}
	prev_row->next = NULL;


}

static
void
insert_value_in_new_cell(uint32_t row, uint32_t col, struct value* value) {


	uint32_t i = 0;

	struct row * curr_row = csv.rows;


	while(curr_row->next != NULL && i < row) {
		curr_row = curr_row->next;
		i++;
	}

	if(i != row && row != 0) {
		//In this case we need to add more rows
		while(i < row) {
			curr_row->next = smalloc(sizeof(struct row));
			curr_row->next->previous = curr_row;
			curr_row = curr_row->next;
			curr_row->next = NULL;
			curr_row->row = NULL;
			curr_row->orig_row = NULL;
			curr_row->added_row = TRUE;
			curr_row->nvals = 0;
			curr_row->orig_nvals = 0;
			i++;
		}
		//At this point curr_row = the row'th row
	}


	i = 0;
	if(curr_row->vals == NULL) {
		curr_row->vals = smalloc(sizeof(struct value));
		curr_row->vals->len = col;
		curr_row->vals->orig_len = col;
		curr_row->vals->val = strdup("");
		curr_row->vals->orig_val = strdup("");
		curr_row->vals->added_val = TRUE;
		curr_row->vals->next = NULL;
		curr_row->vals->previous = NULL;

	}
	struct value* curr_val = curr_row->vals;
	while(curr_val->next != NULL && i < col-1) {
		curr_val = curr_val->next;
		i++;
	}

	if(i != col-1 && col != 0) {
		//We need to add some more empty cols
		while(i < col-1) {
			curr_val->next = smalloc(sizeof(struct value));
			curr_val->next->previous = curr_val;
			curr_val = curr_val->next;
			curr_val->len = 0;
			curr_val->orig_len = 0;
			curr_val->val = strdup("");
			curr_val->orig_val = strdup("");
			curr_val->added_val = TRUE;
			curr_val->next = NULL;
			i++;
		}

	}

	curr_val->next = value;
	value->previous = curr_val;

}




static
void
insert_value(uint32_t row, uint32_t col, char* value) {

	struct value* new_value = smalloc(sizeof(struct value));
	new_value->val = strdup(value);
	new_value->orig_val = strdup(value);
	new_value->len = strlen(value);
	new_value->orig_len = new_value->len;
	new_value->added_val = TRUE;


	struct row * curr_row = csv.rows;


	if(row > csv.nrows) {

		insert_value_in_new_cell(row, col, new_value);
		return;
	}


	//Add something into a new row at the end
	if(row == csv.nrows) {
		while(curr_row->next != NULL) curr_row = curr_row->next;

		curr_row->next = smalloc(sizeof(struct row));
		curr_row->next->previous = curr_row;
		curr_row = curr_row->next;
		curr_row->row = strdup(value);
		curr_row->orig_row = strdup(value);
		curr_row->nvals = 1;
		curr_row->orig_nvals = 1;
		curr_row->added_row = TRUE;
		curr_row->next = NULL;

		curr_row->vals = new_value;
		new_value->previous = NULL;
		new_value->next = NULL;
		return;
	}

	uint32_t i = 0;

	while(curr_row != NULL && i < row) {
		i++;
		curr_row = curr_row->next;
	}
	struct value* curr_val = curr_row->vals;
	if(col > curr_row->nvals) {
		insert_value_in_new_cell(row, col, new_value);
		return;
	}
	if( col == curr_row->nvals) {
		//We are adding a node at the end
		while( curr_val->next != NULL) {
			curr_val = curr_val->next;
		}
		curr_val->next = new_value;
		new_value->previous = curr_val;
	}
	else if (col == 0) {
		curr_row->vals->previous = new_value;
		new_value->next = curr_row->vals;
		new_value->previous = NULL;
		curr_row->vals = new_value;
	}
	else {
		i = 0;
		while (curr_val != NULL && i < col) {
			i++;
			curr_val = curr_val->next;
		}



		curr_val->previous->next = new_value;
		new_value->previous = curr_val->previous;
		new_value->next = curr_val;
		curr_val->previous = new_value;
	}
	curr_row->nvals++;


}

/* For each row in the csv, flip a bias coin to see if we should
 * print it or not */
static
void
fuzz_populate_width(struct state *s)
{
	uint64_t len = 0;

	slseek(s->payload_fd, 0, SEEK_SET);

	struct row * curr_row = csv.rows;

	while(curr_row != NULL) {

		struct value * curr_value = curr_row->vals;
		while(curr_value != NULL) {


			len += swrite(
				s->payload_fd,
				curr_value->val,
				curr_value->len
			   );

			/* Seperate each element by a "," unless it is the last element in
			 * a row, then seperate with a "\n" */
			if (curr_value->next == NULL){
				//We have reached the end of the row
				if (coin_flip(90)) {
					//We will go back to the start of this row and add more to it
					curr_value = curr_row->vals;
					len += swrite(s->payload_fd, ",", 1);
				}
				else {
					//we will finish this row here
					len += swrite(s->payload_fd, "\n", 1);
				}
			}
			else {
				len += swrite(s->payload_fd, ",", 1);
			}

			curr_value = curr_value->next;
		}

		curr_row = curr_row->next;

	}

	sftruncate(s->payload_fd, len);
	deploy();
}

/* For each row in the csv, flip a bias coin to see if we should
 * print it or not */
static
void
fuzz_populate_length(struct state *s)
{

	uint64_t len = 0;

	slseek(s->payload_fd, 0, SEEK_SET);

	struct row * curr_row = csv.rows;

	while(curr_row != NULL) {

		if (coin_flip(90)) {
			len += dump_row(s->payload_fd, curr_row);
		}
		else {
			curr_row = curr_row->next;
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

	struct value * curr_value = row->vals;
	while(curr_value != NULL) {


		len += swrite(
			fd,
			curr_value->val,
			curr_value->len
		   );

		/* Seperate each element by a "," unless it is the last element in
		 * a row, then seperate with a "\n" */
		if (curr_value->next == NULL)
			len += swrite(fd, "\n", 1);
		else
			len += swrite(fd, ",", 1);

		curr_value = curr_value->next;
	}

	return len;
}

/* flip just the control characters */
static
void
fuzz_bit_shift_flip_control_char(struct state *s)
{

	size_t offset, len = dump_csv(s);

	for (size_t i = 0; i < len; i++) {
		char ch = s->mem[i];

		switch (ch) {
		case '\\':
		case '\n':
		case ',':
			offset = i + roll_dice(1, 10); /* Fuzz some neighbouring bytes */
			offset = MIN(offset, len); /* Don't want to fuzz past the file */
			bit_shift_in_range(s->payload_fd, i, offset-i);
			bit_flip_in_range(s->payload_fd, i, offset-i);
			break;
		}
	}
}


/* flip random characters */
static
void
fuzz_bit_shift_flip_chars(struct state *s)
{

	size_t len = dump_csv(s);

	size_t offset = roll_dice(1, len); /* Fuzz some neighbouring bytes */
	size_t range = roll_dice(1, 20);
	range = MIN(offset + range, len) - offset; /* Don't want to fuzz past the file */
	bit_shift_in_range(s->payload_fd, offset, range);
	bit_flip_in_range(s->payload_fd, offset, range);

}


/* Here we look for numbers in our csv file and change then to crazy values */
static
void
fuzz_bad_nums(struct state *s)
{

	struct row * curr_row = csv.rows;
	while (curr_row != NULL) {
		struct value * curr_val = curr_row->vals;
		while(curr_val != NULL) {
			try_bad_nums(curr_val, s);
			curr_val = curr_val->next;
		}

		curr_row = curr_row->next;
	}
}

static
void
fuzz_buffer_overflow(struct state *s)
{


	struct row * curr_row = csv.rows;
	while (curr_row != NULL) {
		struct value * curr_val = curr_row->vals;
		while(curr_val != NULL) {
			try_buffer_overflow(curr_val, s);
			curr_val = curr_val->next;
		}

		curr_row = curr_row->next;
	}
}


/* Will try to fuzz characters if they are implemented into formulas or
some form of active code
https://payatu.com/csv-injection-basic-to-exploit
*/
static
void
fuzz_special_chars(struct state *s) {

	struct row * curr_row = csv.rows;
	while (curr_row != NULL) {
		struct value * curr_val = curr_row->vals;
		while(curr_val != NULL) {
			try_special_chars(curr_val, s);
			curr_val = curr_val->next;
		}

		curr_row = curr_row->next;
	}


}

static
void
try_special_chars(struct value* value_to_fuzz, struct state *s) {

	/* CSV unique strings pulled with inspiration from:
	 * https://payatu.com/csv-injection-basic-to-exploit
	 */
	char *cases[] = {
		"=A1+A2",
		"=Z0+A200",
		"=A-1+22",
		"=#$%^",
		"=",
		"HYPERLINK(plsdonthaq.me, [friendly_name])",
		"HYPERLINK(, [])",
		"cmd|' /C notepad'!'A1'",
		"+-@&;",
	};

	char *old_val = value_to_fuzz->val;
	uint64_t old_len = value_to_fuzz->len;

	for (uint64_t i = 0; i < ARRSIZE(cases); i++) {
		value_to_fuzz->val = cases[i];
		value_to_fuzz->len = strlen(cases[i]);

		dump_csv(s);
		deploy();
	}

	/* General strings from util.h */
	for (uint64_t i = 0; i < ARRSIZE(bad_strings); i++) {

		value_to_fuzz->val = bad_strings[i].s;
		value_to_fuzz->len = bad_strings[i].n;

		dump_csv(s);
		deploy();
	}

	value_to_fuzz->val = old_val;
	value_to_fuzz->len = old_len;
}

static
void
try_bad_nums(struct value* value_to_fuzz, struct state *s)
{


	char *old_val = value_to_fuzz->val;
	uint64_t old_len = value_to_fuzz->len;

	for (uint64_t i = 0; i < ARRSIZE(bad_nums); i++) {
		value_to_fuzz->val =  strdup(bad_nums[i].s);
		value_to_fuzz->len = strlen(bad_nums[i].s);
		dump_csv(s);
		deploy();
	}

	for (uint64_t i = 0; i < ARRSIZE(bad_nums_floats); i++) {
		value_to_fuzz->val =  strdup(bad_nums_floats[i].s);
		value_to_fuzz->len = strlen(bad_nums_floats[i].s);
		dump_csv(s);
		deploy();
	}

	value_to_fuzz->val = old_val;
	value_to_fuzz->len = old_len;

}

/* Write the csv file to state->payload_fd */
static
off_t
dump_csv(struct state *s)
{
	off_t len = 0;

	slseek(s->payload_fd, 0, SEEK_SET);

	struct row * curr_row = csv.rows;
	while(curr_row != NULL) {
		struct value * curr_val = curr_row->vals;
		if(curr_val == NULL) { // This is an empty row
			len += swrite(s->payload_fd, "\n", 1);
		}
		while(curr_val != NULL) {
			len += swrite (
				s->payload_fd,
				curr_val->val,
				curr_val->len
				);
			if (curr_val->next == NULL)
				len += swrite(s->payload_fd, "\n", 1);
			else
				len += swrite(s->payload_fd, ",", 1);

			curr_val = curr_val->next;

		}
		curr_row = curr_row->next;
	}

	sftruncate(s->payload_fd, len);
	return len;
}

static
void
fuzz_empty_cells(struct state *s){


	uint64_t nrows = csv.nrows;
	uint64_t ncols = csv.rows->orig_nvals;

	struct row * curr_row = csv.rows;

	while(curr_row != NULL) {

		struct value * curr_value = curr_row->vals;
		while(curr_value != NULL) {
				if(coin_flip(10)) { //Low as it gets run repeatedly

					uint32_t row = 0;
					uint32_t col = 0;

					if (coin_flip(50)) {
						row = rand()%(nrows*NUM_EMPTY_CELLS_CONSTANT) + nrows;
						col = rand()%(ncols*NUM_EMPTY_CELLS_CONSTANT);
					} else {
						row = rand()%(nrows*NUM_EMPTY_CELLS_CONSTANT);
						col = rand()%(ncols*NUM_EMPTY_CELLS_CONSTANT) + ncols;
					}


					insert_value(row, col, curr_value->val);
					dump_csv(s);
					deploy();
				}

			curr_value = curr_value->next;
		}
		curr_row = curr_row->next;
	}

	revert_csv_data_structures();


}




static
void
try_buffer_overflow(struct value* value_to_fuzz, struct state *s)
{

	free(value_to_fuzz->val);
	value_to_fuzz->val = strdup(BIG);
	value_to_fuzz->len = sizeof(BIG)-1;
	dump_csv(s);
	deploy();
	revert_csv_data_structures();

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
	row->orig_row = strdup(text);
	row->nvals = arr_len((const void **) arr);
	row->orig_nvals = row->nvals;
	row->vals = smalloc(sizeof(struct value));
	row->added_row = FALSE;


	struct value *curr_val = row->vals;
	struct value *prev_value = NULL;


	for (uint64_t i = 0; i < row->nvals; i++) {
		curr_val->val = arr[i];
		curr_val->orig_val = strdup(arr[i]);
		curr_val->len = strlen(arr[i]);
		curr_val->orig_len = curr_val->len;
		curr_val->added_val = FALSE;
		curr_val->next = smalloc(sizeof(struct value));
		curr_val->previous = prev_value;
		prev_value = curr_val;
		curr_val = curr_val->next;
	}
	prev_value->next = NULL;
	free(curr_val);

	free(arr);
}

