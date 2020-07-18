/*******************************************
 *                                         *
 *    Author: Brendan Nyholm (z5206679)    *
 *    Date:   16/07/20 12:30               *
 *                                         *
 *******************************************/



#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>


#include "config.h"
#include "fuzzer.h"
#include "mutation_functions.h"
#include "safe.h"


// void deploy() {
// 	int fileDes = open(TESTDATA_FILE, O_RDONLY);
// 	// printf("%d\n", fileDes);
// 	int len_of_file = lseek(fileDes,0,SEEK_END);
// 	lseek(fileDes,0,SEEK_SET);
// 	char *buf = malloc(len_of_file);

// 	read(fileDes, buf, len_of_file);
// 	printf("Testdata.bin: %s\n", buf);
// 	close(fileDes);
// }

void bit_shift_in_range(int fd, int start_range, int len) {

	lseek(fd, start_range, SEEK_SET);
	int shift = 0;
	char *bytes = malloc(sizeof(char)*len);
	read(fd, bytes, len);
	lseek(fd, start_range, SEEK_SET);
	int i = 0;
	srand(start_range);

	while (i < len) {
		shift = (rand()%7) + 1; //Don't want shift it by 0 bits
		char new_byte = (bytes[i] << shift);

		lseek(fd, start_range+i, SEEK_SET);
		write(fd, &new_byte, 1);
		deploy();

		lseek(fd, start_range+i, SEEK_SET);
		write(fd, &bytes[i], 1);

		i++;

	}

	i = 0;
	while(i < len*NUM_OF_BIT_SHIFT_ITERATIONS_CONSTANT) {
		int byte_to_shift = rand()%len;

		shift = (rand()%7) + 1; //Don't want shift it by 0 bits
		char new_byte = (bytes[byte_to_shift] << shift);

		lseek(fd, start_range+byte_to_shift, SEEK_SET);
		write(fd, &new_byte, 1);

		deploy();

		lseek(fd, start_range+byte_to_shift, SEEK_SET);
		write(fd, &bytes[byte_to_shift], 1);


		i++;

	}

	lseek(fd, start_range, SEEK_SET);
	write(fd, bytes, len);

}





void bit_flip_in_range(int fd, int start_range, int len) {
	lseek(fd, start_range, SEEK_SET);
	unsigned char mask = 255;
	char *bytes = malloc(len);
	read(fd, bytes, len);
	lseek(fd, start_range, SEEK_SET);

	// printf("%c\n",bytes[1]);

	srand(start_range);

	int i = 0;
	while (i < len) {

		char new_byte = (mask^bytes[i]);

		lseek(fd, start_range+i, SEEK_SET);
		write(fd, &new_byte, 1);
		deploy();

		lseek(fd, start_range+i, SEEK_SET);
		write(fd, &bytes[i], 1);

		i++;
	}

	// i = 0;
	// while(i < 15) {
	// 	mask = rand();
	// 	printf("%hhx\n",mask);
	// 	i++;
	// }

	i = 0;
	while (i < NUM_OF_BIT_FLIP_ITERATIONS_CONSTANT*len) {

		mask = rand();

		// printf("The mask is: %hhd\n", mask);

		int byte_to_flip = rand()%(len);

		// printf("byte to flip: %d\n", byte_to_flip);

		char new_byte = (mask^bytes[byte_to_flip]);

		// printf("New byte is %c\n", new_byte);

		lseek(fd, start_range+byte_to_flip, SEEK_SET);
		write(fd, &new_byte, 1);
		deploy();

		lseek(fd, start_range+byte_to_flip, SEEK_SET);
		write(fd, &bytes[byte_to_flip], 1);

		i++;
	}



	lseek(fd, start_range, SEEK_SET);
	write(fd, bytes, len);
}




int file_length(int fd) {
	lseek(fd, 0, SEEK_SET);
	int file_len = lseek(fd,0,SEEK_END);
	return file_len;
}


int valid_num_char(char character) {
	return ('0' <= character) && (character <= '9');
}


int number_end_offset(char* file_string, int file_string_len) {

	char decimal_point = '.';
	char minus_sign = '-';

	int decimal_point_detected = FALSE;
	int num_length = 0;

	if(file_string[0] == minus_sign) {
		num_length++;
	}

	while(num_length < file_string_len) {
		// printf("Checking char: %c\n", file_string[num_length]);
		// printf("num_length = %d < file_string_len = %d\n", num_length, file_string_len);
		if (valid_num_char(file_string[num_length]) == FALSE) {
			// printf("Char is no valid\n");
			if (file_string[num_length] == decimal_point && decimal_point_detected == FALSE) {
				decimal_point_detected = TRUE;
				num_length++;
			}
			else {
				break;
			}
		}
		else {
			num_length++;
		}

	}
	// printf("Finished checking\n");
	return num_length;


}

void write_int_number(int fd, int byte_offset, char* file_contents, int num_length, long number) {
	int file_len = file_length(fd);

	//replace with -1
	lseek(fd, byte_offset, SEEK_SET);
	// printf("Num_length = %d\n", num_length);
	// printf("File contents[num_length] = %s\n", (file_contents+num_length));
	int new_number_length = snprintf( NULL, 0, "%ld", number );
	// printf("New number length = %d\n", new_number_length);
	char * quick_string = calloc((new_number_length+1),sizeof(char));
	snprintf( quick_string, new_number_length+1, "%ld", number);
	// printf("Quick string is: %s\n", quick_string);
	write(fd, quick_string, new_number_length); //Does not include null terminator
	// printf("String is: %s\n",&file_contents[num_length]);
	write(fd, &file_contents[num_length], file_len-byte_offset-num_length);
	// printf("Trucating at char num %d\n", file_len+new_number_length - num_length);
	ftruncate(fd,file_len+new_number_length - num_length);

	deploy();

	lseek(fd, byte_offset, SEEK_SET);
	write(fd, file_contents, file_len-byte_offset);
	ftruncate(fd,file_len);
	free(quick_string);

}

void write_float_number(int fd, int byte_offset, char* file_contents, int num_length, double number) {
	int file_len = file_length(fd);

	//replace with -1
	lseek(fd, byte_offset, SEEK_SET);
	// printf("Num_length = %d\n", num_length);
	// printf("File contents[num_length] = %s\n", (file_contents+num_length));
	int new_number_length = snprintf( NULL, 0, "%g", number );
	// printf("New number length = %d\n", new_number_length);
	char * quick_string = calloc((new_number_length+1),sizeof(char));
	snprintf( quick_string, new_number_length+1, "%g", number);
	// printf("Quick string is: %s\n", quick_string);
	write(fd, quick_string, new_number_length); //Does not include null terminator
	// printf("String is: %s\n",&file_contents[num_length]);
	write(fd, &file_contents[num_length], file_len-byte_offset-num_length);
	ftruncate(fd,file_len+new_number_length - num_length);

	deploy();

	lseek(fd, byte_offset, SEEK_SET);
	write(fd, file_contents, file_len-byte_offset);
	ftruncate(fd,file_len);
	free(quick_string);
}

// byte_offset, the offset of the file for which the number it is, it can be a float or integer
void replace_numbers(int fd, int byte_offset) {
	int file_len = file_length(fd);
	slseek(fd, byte_offset, SEEK_SET);



	char* file_contents = smalloc(file_len- byte_offset);
	sread(fd, file_contents, file_len-byte_offset);

	double number = 0;

	sscanf(file_contents,"%lf", &number);

	int num_length = number_end_offset(file_contents, file_len-byte_offset);

	printf("%lf\n", number);

	write_int_number(fd, byte_offset, file_contents, num_length, 0);
	write_int_number(fd, byte_offset, file_contents, num_length, 1);
	write_int_number(fd, byte_offset, file_contents, num_length, -1);
	write_int_number(fd, byte_offset, file_contents, num_length, -2);
	write_int_number(fd, byte_offset, file_contents, num_length, abs((int)number));


	write_int_number(fd, byte_offset, file_contents, num_length, CHAR_MIN-1);
	write_int_number(fd, byte_offset, file_contents, num_length, CHAR_MAX+1);
	write_int_number(fd, byte_offset, file_contents, num_length, UCHAR_MAX+1);
	write_int_number(fd, byte_offset, file_contents, num_length, 100);
	write_int_number(fd, byte_offset, file_contents, num_length, INT_MIN);
	write_int_number(fd, byte_offset, file_contents, num_length, (long)INT_MIN-1);
	write_int_number(fd, byte_offset, file_contents, num_length, INT_MAX);
	write_int_number(fd, byte_offset, file_contents, num_length, (long)INT_MAX+1);

	write_float_number(fd, byte_offset, file_contents, num_length, 0.1);
	write_float_number(fd, byte_offset, file_contents, num_length, -0.1);
	write_float_number(fd, byte_offset, file_contents, num_length, (double)1/3);
	write_float_number(fd, byte_offset, file_contents, num_length, M_PI);


	slseek(fd, byte_offset, SEEK_SET);
	swrite(fd, file_contents, file_len-byte_offset);
	sftruncate(fd,file_len);
	free(file_contents);


}

//Write a string at offset
void write_string(int fd, int byte_offset, int bytes_len, char* replacement_chars, int replacement_chars_len) {
	int file_len = file_length(fd);

	lseek(fd, byte_offset,SEEK_SET);

	//removed_file_contents includes the bytes between byte_offset and byte_offset+bytes_len
	// we save all these bytes as we wish to rewrite them back to the file to restore it's state
	char* removed_file_contents = malloc(file_len- byte_offset);
	read(fd, removed_file_contents, file_len-byte_offset);

	//We only only want to write the bytes after bytes_offset+bytes_len back to the file
	// as we are replacing the bytes between byte_offset:byte_offset+bytes_len
	// so we shift the second pointer that much further into the string
	char * string_to_restore = removed_file_contents+bytes_len;

	lseek(fd, byte_offset, SEEK_SET);

	write(fd, replacement_chars, replacement_chars_len);

	write(fd, string_to_restore, file_len- byte_offset+bytes_len);

	ftruncate(fd, file_len - bytes_len + replacement_chars_len);

	deploy();



	lseek(fd, byte_offset, SEEK_SET);
	write(fd, removed_file_contents, file_len-byte_offset);
	ftruncate(fd,file_len);
	free(removed_file_contents);


}

void replace_strings(int fd, int byte_offset, int replace_str_len) {

	//strings to test
	char format_string[] = "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s";
	int format_string_len = strlen(format_string);
	char format_string2[] = "%n%n%n%n%n%n%n%n%n%n%n%n%n%n%n";
	int format_string2_len = strlen(format_string2);
	char buffer_overflow[1000];
	memset(buffer_overflow, 'A', 1000);
	int buffer_overflow_len = 1000;
	char newline_break[] = "\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA\nA";
	int newline_break_len = strlen(newline_break);
	char null_ptr_break[] = "\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A\0A";
	int null_ptr_break_len = 96;
	char new_line_null_ptr[] = "\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n\0\n";
	int new_line_null_ptr_len = 70;
	char empty_string[] = "";
	int empty_string_len = 0;

	write_string(fd, byte_offset, replace_str_len, format_string, format_string_len);

	write_string(fd, byte_offset, replace_str_len, format_string2, format_string2_len);

	write_string(fd, byte_offset, replace_str_len, buffer_overflow, buffer_overflow_len);

	write_string(fd, byte_offset, replace_str_len, newline_break, newline_break_len);

	write_string(fd, byte_offset, replace_str_len, null_ptr_break, null_ptr_break_len);

	write_string(fd, byte_offset, replace_str_len, new_line_null_ptr, new_line_null_ptr_len);

	write_string(fd, byte_offset, replace_str_len, empty_string, empty_string_len);

	return;

}





// int main (int argc, char *argv[]) {
// 	// printf("%s\n",argv[1]);
// 	int fileDes = open(TESTDATA_FILE, O_RDWR);
// 	// printf("The bit mask is: \n");
// 	// printf("%d\n", fileDes);
// 	bit_shift_in_range(fileDes, 0, 20);
// 	// bit_flip_in_range(fileDes, 14, 5);
// 	// replace_numbers(fileDes, 14);
// 	// replace_strings(fileDes, 14, 21);




// }

