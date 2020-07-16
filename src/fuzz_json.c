#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "fuzzer.h"
#include "safe.h"
#include "utils.h"
#include "json.h"
#include "json-builder.h"

static struct {

	json_value *jv;

	/* json-builder likes to write to a chunk of memory  */
	uint64_t memlen;
	char *mem;

} json = {0};

void
fuzz_handle_json(struct state *state)
{

	json_settings settings = {};
	settings.value_extra = json_builder_extra;
	char err[json_error_max];
	json.jv = json_parse_ex(&settings, state->mem, state->stat.st_size, err);
	if (json.jv == NULL)
		panic("Failed to init json parser\n");

	json.memlen = (json_measure(json.jv) + sizeof(BIG)) * 5;
	json.mem = smalloc(json.memlen);

	json_serialize(json.mem, json.jv);
	printf("%s\n", json.mem);
}
