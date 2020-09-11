#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "fs.h"
#include "fuzz_plaintext.h"
#include "fuzzer.h"
#include "mutations.h"
#include "safe.h"
#include "utils.h"

static struct {
	/* Number of lines */
	uint64_t nlines;

	char **lines;
} pt = {0};

/* Helper functions */
static void try_buffer_overflow(struct state *s);
static uint64_t get_nlines(struct state *s);
static bool is_int(const char c);
static off_t pt_dump(struct state *s);
static void fuzz_bad_strings(struct state *s);
static void fuzz_bad_ints(struct state *s);
static void fuzz_flip_shifts(struct state *s, uint64_t n, char *l);
static void fuzz(struct state *s);
static void fuzz_shift_all_lines(struct state *s);

/* Here are the functions that can be run multiple times and
   do different things each time. */
static void (*fuzz_payloads_repeat[])(struct state *) = {
	fuzz_shift_all_lines,
};

/* Here are the functions that should be only run once */
static void (*fuzz_payloads_single[])(struct state *) = {
	try_buffer_overflow,
	fuzz_bad_ints,
	fuzz_bad_strings,
};

void
fuzz_handle_plaintext(struct state *state)
{
	pt.nlines = get_nlines(state);

	pt.lines = smalloc(pt.nlines * sizeof(char*));

	char * token = strtok(state->mem, "\n");
	// loop through the string to extract all other tokens
	int i = 0;
	while(token != NULL) {
		uint64_t line_len = strlen(token);
		pt.lines[i] = smalloc(line_len+2);

		strcpy(pt.lines[i], token);
		pt.lines[i][line_len] = '\n';
		token = strtok(NULL, "\n");
		i++;
	}

	fuzz(state);
}

void
free_handle_plaintext(UNUSED struct state *s)
{
	for (uint64_t i = 0; i < pt.nlines; i++) {
		free(pt.lines[i]);
	}
	free(pt.lines);
}

static
void
fuzz(struct state *s)
{
	for (uint64_t i = 0; i < ARRSIZE(fuzz_payloads_single); i++)
		fuzz_payloads_single[i](s);

	while (1) {
		uint32_t idx = roll_dice(0, ARRSIZE(fuzz_payloads_repeat)-1);
		fuzz_payloads_repeat[idx](s);
	}
}

static
void
fuzz_shift_all_lines(struct state *s)
{
	uint64_t prev_bytes = 0;
	for(uint64_t j=0; j < pt.nlines; j++){

		for (uint64_t i = roll_dice(pt.nlines, pt.nlines + 1337); i; i--)
			fuzz_flip_shifts(s, prev_bytes, pt.lines[j]);

		prev_bytes = prev_bytes + strlen(pt.lines[j]);
	}
}

static
void
fuzz_flip_shifts(struct state *s, uint64_t n, char *l)
{
	bit_shift_in_range(s->payload_fd, n, strlen(l));
	bit_flip_in_range(s->payload_fd, n, strlen(l));
}


/* Not really a buffer overflow, but just print the file a whole bunch
 * of times. */
static
void
try_buffer_overflow(struct state *s)
{
	off_t len = 0;
	uint64_t niters = roll_dice(pt.nlines, pt.nlines + 1337);

	for (uint64_t i = 0; i < niters; i++)
		len += swrite(s->payload_fd, s->mem, s->stat.st_size);

	sftruncate(s->payload_fd, len);
	deploy();
}

static
void
fuzz_bad_strings(struct state *s)
{
	pt_dump(s);
	uint64_t prev_bytes = 0;
	for (uint64_t i = 0; i < pt.nlines; i++) {
		replace_strings(s->payload_fd, prev_bytes, strlen(pt.lines[i]));
		prev_bytes += strlen(pt.lines[i]);
	}
}

static
void
fuzz_bad_ints(struct state *s)
{
	pt_dump(s);
	uint64_t prev_bytes = 0;
	for(uint64_t i=0; i < pt.nlines; i++){
		if (is_int(pt.lines[i][0])) {
			replace_numbers(s->payload_fd, prev_bytes);
		}
		prev_bytes += strlen(pt.lines[i]);
	}
}

static
bool
is_int(const char c)
{
	return ('0' <= c && c <= '9');
}

static
uint64_t
get_nlines(struct state *s)
{
	uint64_t ret = 0;
	for (off_t i = 0; i < s->stat.st_size; i++) {
		if (s->mem[i] == '\n')
			ret++;
	}
	return MAX(ret,1);
}

static
off_t
pt_dump(struct state *s)
{
	slseek(s->payload_fd, 0, SEEK_SET);

	off_t ret = 0;
	for (uint64_t i = 0; i < pt.nlines; i++)
		ret += swrite(s->payload_fd, pt.lines[i], strlen(pt.lines[i]));

	sftruncate(s->payload_fd, ret);

	return ret;
}

