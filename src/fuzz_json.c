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
#include "mutation_functions.h"

static struct {

	json_value *jv;

	/* json-builder likes to write to a chunk of memory  */
	uint64_t memlen;
	char *mem;

} json = {0};

/* Helper functions */
static void fuzz(struct state *s);
static void fuzz_buffer_overflow(struct state *s);
static const char *debug_get_type(json_type jt);
static void json_dump(struct state *s);

/* Here we add functions used for fuzzing.
 * Each function tests for something different. */
static void (*fuzz_payloads[])(struct state *) = {
	fuzz_buffer_overflow,
};

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

	fuzz(state);
}

/* Does the actual fuzzing */
static
void
fuzz(struct state *s)
{
	/* XXX We might want a loop here? */
	//Example of a call
	printf("Hello");
	bit_shift_in_range(s->payload_fd, 0 ,0);
	fuzz_payloads[0](s);
}

/* TODO Patch this so it traverses the json file recursevly,
 * so far we are just buffer overlowing the keys*/
static
void
fuzz_buffer_overflow(struct state *s)
{
	unsigned int i;

	if (json.jv->type != json_object)
		panic(
			"This function does not now how to handle this type: %s\n",
			json.jv->type
		);

	/* We change the value of each key to BIG then restore it */
	for (i = 0; i < json.jv->u.object.length; i++) {

		json_object_entry *entry = &(json.jv->u.object.values[i]);

		json_char *old_name = entry->name;
		unsigned int old_len = entry->name_length;

		entry->name = BIG;
		entry->name_length = sizeof(BIG)-1;

		json_dump(s);
		deploy();

		entry->name = old_name;
		entry->name_length = old_len;
	}
}

/* Dump the json_value into the payload_fd file */
static
void
json_dump(struct state *s)
{
	size_t len = json_measure(json.jv);
	if (len > json.memlen)
		panic("The json object is larger than the size of memory");
	json_serialize(json.mem, json.jv);
	slseek(s->payload_fd, 0, SEEK_SET);
	swrite(s->payload_fd, json.mem, len);
	sftruncate(s->payload_fd, len);
}

UNUSED
static
const char *
debug_get_type(json_type jt)
{
	switch(jt) {
	case json_none: return "json_none";
	case json_object: return "json_object";
	case json_array: return "json_array";
	case json_integer: return "json_integer";
	case json_double: return "json_double";
	case json_string: return "json_string";
	case json_boolean: return "json_boolean";
	case json_null: return "json_null";
	default: panic("Unkown json_type: %d\n", jt);
	}
}

