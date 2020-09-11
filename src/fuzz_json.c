#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "fs.h"
#include "fuzzer.h"
#include "json-builder.h"
#include "json.h"
#include "mutations.h"
#include "safe.h"
#include "utils.h"

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
static size_t json_dump(struct state *s);
static void traverse_json(json_value *jv, joe_handle joeh, jv_handle jvh);
static void fuzz_bad_nums(struct state *s);
static void traverse_json_object(json_value *, joe_handle, jv_handle);
static void traverse_json_array(json_value *, joe_handle, jv_handle);
static void fuzz_fmt_str(struct state *s);
static void fuzz_bit_shift(struct state *s);
static void fuzz_empty(struct state *s);
static void fuzz_extra_entries(struct state *s);
static void fuzz_extra_objects(struct state *s);
static void fuzz_append_objects(struct state *s);

/* Here are the functions that can be run multiple times and
   do different things each time. */
static void (*fuzz_payloads_repeat[])(struct state *) = {
	fuzz_bit_shift,
};

/* Here are the functions that should be only run once */
static void (*fuzz_payloads_single[])(struct state *) = {
	fuzz_buffer_overflow,
	fuzz_bad_nums,
	fuzz_fmt_str,
	fuzz_empty,
	fuzz_extra_entries,
	fuzz_extra_objects,
	fuzz_append_objects,
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

void
free_handle_json(UNUSED struct state *state)
{
	free(json.mem);
	json_value_free(json.jv);
}

/* Does the actual fuzzing */
static
void
fuzz(struct state *s)
{

	for (uint32_t i = 0; i < ARRSIZE(fuzz_payloads_single); i++) {
		fuzz_payloads_single[i](s);
	}

	while (1) {
		uint32_t idx = roll_dice(0, ARRSIZE(fuzz_payloads_repeat)-1);
		fuzz_payloads_repeat[idx](s);
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
	if (jvh)
		jvh(jv);

	if (jv->type != json_object && jv->type != json_array)
		return; /* We only recurse on certain json objects */

	if (jv->type == json_object) {
		traverse_json_object(jv, joeh, jvh);
	} else if (jv->type == json_array) {
		traverse_json_array(jv, joeh, jvh);
	}
}

static
void
traverse_json_object(json_value *jv, joe_handle joeh, jv_handle jvh)
{
	assert(jv->type == json_object);

	for (unsigned int i = 0; i < jv->u.object.length; i++) {
		json_object_entry *entry = &(jv->u.object.values[i]);
		json_value *value = jv->u.object.values[i].value;

		if (joeh)
			joeh(entry);

		traverse_json(value, joeh, jvh);
	}
}

static
void
traverse_json_array(json_value *jv, joe_handle joeh, jv_handle jvh)
{
	assert(jv->type == json_array);

	for (unsigned int i = 0; i < jv->u.object.length; i++) {
		json_value *value = jv->u.array.values[i];

		traverse_json(value, joeh, jvh);
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

/* Dump the json_value into the payload_fd file, return the length of the json
 * object */
static
size_t
json_dump(struct state *s)
{
	size_t len = json_measure(json.jv);
	if (len > json.memlen)
		panic("The json object is larger than the size of memory");
	json_serialize(json.mem, json.jv);
	slseek(s->payload_fd, 0, SEEK_SET);
	swrite(s->payload_fd, json.mem, len);
	sftruncate(s->payload_fd, len);
	return len;
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
		} else if (jv->type == json_double) {
			for (uint32_t i = 0; i < ARRSIZE(bad_nums_floats); i++) {
				double old_double = jv->u.dbl;

				jv->u.dbl = bad_nums_floats[i].n;

				json_dump(s);
				deploy();

				jv->u.dbl = old_double;
			}
		}
	}
	traverse_json(json.jv, NULL, &f);
}

static
void
fuzz_fmt_str(struct state *s)
{
	/* Set json_strings to known format strings */
	void
	f1(json_value *jv)
	{
		if (jv->type != json_string)
			return;

		json_char *old_str = jv->u.string.ptr;
		unsigned int old_len = jv->u.string.length;

		for (uint32_t i = 0; i < ARRSIZE(fmt_strings); i++) {

			jv->u.string.ptr = (char *) fmt_strings[i];
			jv->u.string.length = strlen(fmt_strings[i]);

			json_dump(s);
			deploy();
		}

		jv->u.string.ptr = old_str;
		jv->u.string.length = old_len;
	}

	/* Set entries to known format strings */
	void
	f2(json_object_entry *entry)
	{
		json_char *old_name = entry->name;
		unsigned int old_len = entry->name_length;

		for (uint32_t i = 0; i < ARRSIZE(fmt_strings); i++) {
			entry->name = (char *) fmt_strings[i];
			entry->name_length = strlen(fmt_strings[i]);

			json_dump(s);
			deploy();
		}

		entry->name = old_name;
		entry->name_length = old_len;
	}

	traverse_json(json.jv, NULL, &f1);
	traverse_json(json.jv, &f2, NULL);
}

static
void
fuzz_bit_shift(struct state *s)
{
	size_t offset, len = json_dump(s);

	for (uint64_t i = 0; i < len; i++) {
		char ch = json.mem[i];

		switch (ch) {
		case '\\':
		case '\n':
		case '"':
		case ',':
		case '/':
		case ':':
		case '[':
		case ']':
		case '{':
		case '}':
			offset = i + roll_dice(1, 10); /* Fuzz some neighbouring bytes */
			offset = MIN(offset, len); /* Don't want to fuzz past the file */
			bit_shift_in_range(s->payload_fd, i, offset-i);
			break;
		}
	}
}

/*Zero out and empty entries and values*/
static
void
fuzz_empty(struct state *s)
{
	void
	f3(json_object_entry *entry)
	{
		json_char *old_name = entry->name;
		unsigned int old_len = entry->name_length;
		json_value *old_value = entry->value;

		entry->name = "";
		json_dump(s);
		deploy();

		entry->name_length = 0;
		json_dump(s);
		deploy();

		// Might need to change this later
		entry->value = NULL;
		json_dump(s);
		deploy();

		entry->name = old_name;
		entry->name_length = old_len;
		entry->value = old_value;
	}

	traverse_json(json.jv, &f3, NULL);
}

/*Create extra entries*/
static
void
fuzz_extra_entries(struct state *s)
{
	uint32_t length = json.jv->u.object.length;
	for (uint32_t i = 0; i < 100; i++) {
		json_object_push(json.jv, "extra", json_string_new ("extra_value"));
	}

	json_dump(s);
	deploy();

	// Only restore the original entries
	json_value *new_jv = json_object_new(length);
	for (uint32_t i = 0; i < length; i++) {
		json_object_entry *entry = &(json.jv->u.object.values[i]);
		json_object_push(new_jv, entry->name, entry->value);
	}

	// restoring json.jv
	json.jv = new_jv;
}

/*Append extra objects*/
static
void
fuzz_extra_objects(struct state *s)
{
	char * buf2 = malloc(json_measure(json.jv));
    json_serialize(buf2, json.jv);

	char * final = smalloc(105*strlen(buf2));
	strcat(final, "[");
	for (uint32_t i = 0; i < 100; i++) {
		strcat(final, buf2);
		strcat(final, ", ");
	}

	strcat(final, buf2);
	strcat(final, "]");

	slseek(s->payload_fd, 0, SEEK_SET);
	swrite(s->payload_fd, final, strlen(final));
	sftruncate(s->payload_fd, strlen(final));

	deploy();
}

/*Writing json.jv 100 times*/
static
void
fuzz_append_objects(struct state *s)
{
	char * buf2 = malloc(json_measure(json.jv));
    json_serialize(buf2, json.jv);
	char * final = smalloc(105*strlen(buf2));

	for (uint32_t i = 0; i < 100; i++) {
		strcat(final, buf2);
		strcat(final, "\n");
	}

	strcat(final, buf2);
	slseek(s->payload_fd, 0, SEEK_SET);
	swrite(s->payload_fd, final, strlen(final));
	sftruncate(s->payload_fd, strlen(final));
	deploy();
}

