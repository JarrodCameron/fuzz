#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "fuzzer.h"
#include "safe.h"
#include "utils.h"
#include "json.h"

static json_value *json;

void
fuzz_handle_json(struct state *state)
{
	json = json_parse(state->mem, state->stat.st_size);
	if (json == NULL)
		panic("Failed to init json parser\n");

	(void) state;
}
