#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fs.h"
#include "config.h"
#include "fuzzer.h"
#include "mutations.h"
#include "safe.h"

/* Helper functions */
static void write_string(int, uint64_t, uint64_t, char *, uint64_t);
static off_t file_length(int);
static int valid_num_char(char);
static uint64_t number_end_offset(char *, uint64_t);
static void write_int_number(int, uint64_t, char *, uint64_t, long);
static void write_float_number(int, uint64_t, char *, uint64_t, double);

void
bit_shift_in_range(int fd, uint64_t start_range, uint64_t len)
{
	char *bytes = smalloc(len);

	slseek(fd, start_range, SEEK_SET);
	sread(fd, bytes, len);
	slseek(fd, start_range, SEEK_SET);

	for (uint64_t i = 0; i < len; i++) {

		uint64_t shift = roll_dice(1, 7);
		char new_byte = (bytes[i] << shift);

		slseek(fd, start_range+i, SEEK_SET);
		swrite(fd, &new_byte, 1);

		deploy();

		slseek(fd, start_range+i, SEEK_SET);
		swrite(fd, &bytes[i], 1);

	}

	for (uint64_t i = 0; i < len * NBITSHIFTS; i++) {

		uint64_t byte_to_shift = roll_dice(0, len-1);

		uint64_t shift = roll_dice(1, 7);
		char new_byte = (bytes[byte_to_shift] << shift);

		slseek(fd, start_range+byte_to_shift, SEEK_SET);
		swrite(fd, &new_byte, 1);

		deploy();

		slseek(fd, start_range+byte_to_shift, SEEK_SET);
		swrite(fd, &bytes[byte_to_shift], 1);

	}

	slseek(fd, start_range, SEEK_SET);
	swrite(fd, bytes, len);
	free(bytes);
}

void
bit_flip_in_range(int fd, uint64_t start_range, uint64_t len)
{
	char *bytes = smalloc(len);

	slseek(fd, start_range, SEEK_SET);
	sread(fd, bytes, len);
	slseek(fd, start_range, SEEK_SET);

	for (uint64_t i = 0; i < len; i++) {

		char new_byte = (0xff ^ bytes[i]);

		slseek(fd, start_range+i, SEEK_SET);
		swrite(fd, &new_byte, 1);

		deploy();

		slseek(fd, start_range+i, SEEK_SET);
		swrite(fd, &bytes[i], 1);

	}

	for (uint64_t i = 0; i < NBITFLIPS*len; i++) {

		uint64_t byte_to_flip = roll_dice(0, len-1);

		char new_byte = (rand() ^ bytes[byte_to_flip]);

		slseek(fd, start_range+byte_to_flip, SEEK_SET);
		swrite(fd, &new_byte, 1);

		deploy();

		slseek(fd, start_range+byte_to_flip, SEEK_SET);
		swrite(fd, &bytes[byte_to_flip], 1);
	}

	slseek(fd, start_range, SEEK_SET);
	swrite(fd, bytes, len);
	free(bytes);
}

/* byte_offset, the offset of the file for which the number it is, it can be a
 * float or integer */
void
replace_numbers(int fd, uint64_t byte_offset)
{

	off_t file_len = file_length(fd);
	uint64_t buf_size = MAX(file_len-byte_offset, sizeof(double));

	char *file_contents = smalloc(buf_size);
	double number;
	uint64_t num_length;

	slseek(fd, byte_offset, SEEK_SET);
	sread(fd, file_contents, buf_size);

	sscanf(file_contents, "%lf", &number);

	num_length = number_end_offset(file_contents, file_len-byte_offset);

	write_int_number(fd, byte_offset, file_contents, num_length, labs((long)number));

	for (uint64_t i = 0; i < ARRSIZE(bad_nums); i++) {
		write_float_number(
			fd,
			byte_offset,
			file_contents,
			num_length,
			bad_nums[i].n
		);
	}

	write_float_number(fd, byte_offset, file_contents, num_length, 0.1);
	write_float_number(fd, byte_offset, file_contents, num_length, -0.1);
	write_float_number(fd, byte_offset, file_contents, num_length, (double)1/3);
	write_float_number(fd, byte_offset, file_contents, num_length, M_PI);

	slseek(fd, byte_offset, SEEK_SET);
	swrite(fd, file_contents, file_len-byte_offset);
	sftruncate(fd,file_len);
	free(file_contents);
}

void
replace_strings(int fd, uint64_t byte_offset, uint64_t replace_str_len)
{
	for (uint64_t i = 0; i < ARRSIZE(bad_strings); i++) {
		write_string(
			fd,
			byte_offset,
			replace_str_len,
			bad_strings[i].s,
			bad_strings[i].n
		);
	}
}

/* Write a string at offset */
static
void
write_string
(
	int fd,
	uint64_t byte_offset,
	uint64_t bytes_len,
	char *replacement_chars,
	uint64_t replacement_chars_len
)
{
	off_t file_len = file_length(fd);

	slseek(fd, byte_offset,SEEK_SET);

	/* removed_file_contents() includes the bytes between "byte_offset" and
	 * "byte_offset + bytes_len" we save all these bytes as we wish to rewrite
	 * them back to the file to restore it's state */
	char *removed_file_contents = smalloc(file_len - byte_offset);
	sread(fd, removed_file_contents, file_len-byte_offset);

	/* We only only want to write the bytes after "bytes_offset + bytes_len"
	 * back to the file as we are replacing the bytes between
	 * "byte_offset:byte_offset + bytes_len" so we shift the second pointer
	 * that much further into the string */
	char *string_to_restore = removed_file_contents + bytes_len;

	slseek(fd, byte_offset, SEEK_SET);
	swrite(fd, replacement_chars, replacement_chars_len);

	swrite(fd, string_to_restore, file_len - byte_offset - bytes_len);

	sftruncate(fd, file_len - bytes_len + replacement_chars_len);

	deploy();

	slseek(fd, byte_offset, SEEK_SET);
	swrite(fd, removed_file_contents, file_len-byte_offset);
	sftruncate(fd,file_len);
	free(removed_file_contents);
}

static
off_t
file_length(int fd)
{
	struct stat s;
	sfstat(fd, &s);
	return s.st_size;
}

static
int
valid_num_char(char character)
{
	return ('0' <= character) && (character <= '9');
}

static
uint64_t
number_end_offset(char *file_string, uint64_t file_string_len)
{

	uint64_t num_length = 0;
	bool decimal_point_detected = false;

	if (file_string[0] == '-') {
		num_length++;
	}

	while(num_length < file_string_len) {
		if (valid_num_char(file_string[num_length]) == false) {
			if (file_string[num_length] == '.' && !decimal_point_detected) {
				decimal_point_detected = false;
				num_length++;
			} else {
				break;
			}
		} else {
			num_length++;
		}

	}

	return num_length;
}

static
void
write_int_number
(
	int fd,
	uint64_t byte_offset,
	char *file_contents,
	uint64_t num_length,
	long number
)
{
	off_t file_len = file_length(fd);
	slseek(fd, byte_offset, SEEK_SET);
	uint64_t new_number_length = snprintf(NULL, 0, "%ld", number );
	char *quick_string = smalloc(new_number_length + 1);
	snprintf(quick_string, new_number_length + 1, "%ld", number);

	/* Does not include null terminator */
	swrite(fd, quick_string, new_number_length);
	swrite(fd, &file_contents[num_length], file_len - byte_offset-num_length);
	sftruncate(fd,file_len+new_number_length - num_length);

	deploy();

	slseek(fd, byte_offset, SEEK_SET);
	swrite(fd, file_contents, file_len-byte_offset);
	sftruncate(fd,file_len);
	free(quick_string);

}

static
void
write_float_number
(
	int fd,
	uint64_t byte_offset,
	char *file_contents,
	uint64_t num_length,
	double number
)
{
	off_t file_len = file_length(fd);

	/* replace with -1 */
	slseek(fd, byte_offset, SEEK_SET);
	uint64_t new_number_length = snprintf( NULL, 0, "%g", number );
	char *quick_string = smalloc(new_number_length + 1);
	snprintf( quick_string, new_number_length+1, "%g", number);

	/* Does not include null terminator */
	swrite(fd, quick_string, new_number_length);
	swrite(fd, &file_contents[num_length], file_len-byte_offset-num_length);
	ftruncate(fd,file_len+new_number_length - num_length);

	deploy();

	slseek(fd, byte_offset, SEEK_SET);
	swrite(fd, file_contents, file_len-byte_offset);
	sftruncate(fd,file_len);
	free(quick_string);
}


























