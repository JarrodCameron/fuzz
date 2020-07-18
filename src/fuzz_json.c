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

/* Handles for the traverse_json function */
typedef void (*joe_handle)(json_object_entry *);
typedef void (*jv_handle)(json_value *);

/* Helper functions */
static void fuzz(struct state *s);
static void fuzz_buffer_overflow(struct state *s);
static void json_dump(struct state *s);
static void traverse_json(json_value *jv, joe_handle joeh, jv_handle jvh);
static void fuzz_bad_nums(struct state *s);

/* Here we add functions used for fuzzing.
 * Each function tests for something different. */
static void (*fuzz_payloads[])(struct state *) = {
	fuzz_buffer_overflow,
	fuzz_bad_nums,
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
	//Example of a call
	bit_shift_in_range(s->payload_fd, 0 ,0);
	while (1) {
		uint32_t idx = roll_dice(0, ARRSIZE(fuzz_payloads)-1);
		fuzz_payloads[idx](s);
	}
}

/* This function traverses the json object and invokes joeh() and jvh() on each
 * respective object.
 *
 * This provides a generic way of touching each element in the json file.
 * This function should be called as such:
 *              traverse_json(json.jv, &joeh, &jvh);
 *
 * `joeh` and `jvh` can be NULL.
 */
static
void
traverse_json(json_value *jv, joe_handle joeh, jv_handle jvh)
{
	unsigned int i, j;

	if (jv->type != json_object)
		panic(
			"This function does not now how to handle this type: %s\n",
			jv->type
		);

	for (i = 0; i < jv->u.object.length; i++) {
		json_object_entry *entry = &(jv->u.object.values[i]);
		if (joeh)
			joeh(entry);

		/* TODO make handler for: json_none */
		/* TODO make handler for: json_double */
		/* TODO make handler for: json_boolean */
		/* TODO make handler for: json_null */

		/* XXX This MIGHT segfault if entry->value is NULL,
		 * I am not sure if it is possible to have an entry without a
		 *   corrosponding value, if so then this WILL segfault */
		switch (entry->value->type) {
		case json_string:
		case json_integer:
			if (jvh)
				jvh(entry->value);
			break;

		case json_array:
			for (j = 0; j < entry->value->u.array.length; j++) {
				if (jvh)
					jvh(entry->value); /* TODO This should recurse */
			}
			break;

		case json_object:
			traverse_json(entry->value, joeh, jvh);
			break;

		default:
			panic("Add a case for: %s\n", json_type_str(entry->value->type));
		}
	}
}

static
void
fuzz_buffer_overflow(struct state *s)
{
	void
	f(json_object_entry *entry)
	{
		json_char *old_name = entry->name;
		unsigned int old_len = entry->name_length;

		entry->name = BIG;
		entry->name_length = sizeof(BIG)-1;

		json_dump(s);
		deploy();

		entry->name = old_name;
		entry->name_length = old_len;
	}
	traverse_json(json.jv, &f, NULL);
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

static
void
fuzz_bad_nums(struct state *s)
{
	void
	f(json_value *jv)
	{
		if (jv->type == json_integer) {
			for (uint32_t i = 0; i < ARRSIZE(bad_nums); i++) {
				json_int_t old_int = jv->u.integer;

				jv->u.integer = bad_nums[i].n;

				json_dump(s);
				deploy();

				jv->u.integer = old_int;
			}
		}
	}
	traverse_json(json.jv, NULL, &f);
}

