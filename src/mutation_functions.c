#include<stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>


#define TRUE 1
#define FALSE 0

#define NUM_OF_BIT_FLIP_ITERATIONS_CONSTANT 4

//prototype 
// void bit_flip(int fd, int start_range, int end-range)


//Issues
//Currently leaves 0s at the end of the file if the length of the file has been reduced with replace numbers
//Need a different strategy for bit flipping at 255^num bits is 60 seconds for flipping 3 bytes, duplicates also appear

void deploy() {
	int fileDes = open("testdata.bin", O_RDONLY);
	// printf("%d\n", fileDes);
	int len_of_file = lseek(fileDes,0,SEEK_END);
	lseek(fileDes,0,SEEK_SET);
	char *buf = malloc(len_of_file);

	read(fileDes, buf, len_of_file);
	printf("Testdata.bin: %s\n", buf);
	close(fileDes);
}

void bit_shift_in_range(int fd, int start_range, int end_range) {
	if (start_range > end_range) {
		deploy();
		return;
	}
	lseek(fd, start_range, SEEK_SET);
	int shift = 0;
	char byte = 0;
	read(fd, &byte, 1);
	lseek(fd, start_range, SEEK_SET);

	while (shift < 8) {
		char new_byte = (byte << shift);
		if (new_byte == 0) {
			return;
		}
		lseek(fd, start_range, SEEK_SET);
		write(fd, &new_byte, 1);
		bit_shift_in_range(fd, start_range+1, end_range);
		shift++;
	}
	lseek(fd, start_range, SEEK_SET);
	write(fd, &byte, 1);

}


// void bit_flip_in_range(int fd, int start_range, int end_range) {

// 	if(start_range > end_range) {
// 		deploy();
// 		return;
// 	}
// 	lseek(fd, start_range, SEEK_SET);
// 	unsigned char mask = 0;
// 	char byte = 0;
// 	read(fd, &byte, 1);
// 	lseek(fd, start_range, SEEK_SET);


// 	int iterations = 0;
// 	while (iterations < 256) {
// 		// printf("%d, ", mask);
// 		char new_byte = (mask^byte);
// 		// printf("The range is %d - %d\n", start_range, end_range);
// 		// if(new_byte == 0) {
// 		// 	printf("new_byte is 0\n");
// 		// }
// 		lseek(fd, start_range, SEEK_SET);
// 		write(fd, &new_byte, 1);
// 		bit_flip_in_range(fd, start_range+1, end_range);
// 		mask++;
// 		iterations++;
// 	}

// 	lseek(fd, start_range, SEEK_SET);
// 	write(fd, &byte, 1);
// 	// deploy()

// }

void bit_flip_in_range(int fd, int start_range, int len) {
	lseek(fd, start_range, SEEK_SET);
	unsigned char mask = 255;
	char *bytes = malloc(len);
	read(fd, bytes, len);
	lseek(fd, start_range, SEEK_SET);

	// printf("%c\n",bytes[1]);

	srand(start_range);

	int i = 0;
	// while (i < len) {

	// 	char new_byte = (mask^bytes[i]);

	// 	lseek(fd, start_range+i, SEEK_SET);
	// 	write(fd, &new_byte, 1);
	// 	deploy();

	// 	lseek(fd, start_range+i, SEEK_SET);
	// 	write(fd, &bytes[i], 1);

	// 	i++;
	// }

	i = 0;
	while (i < NUM_OF_BIT_FLIP_ITERATIONS_CONSTANT*len) {

		mask = rand();

		printf("%d\n", mask);

		int byte_to_flip = rand()%(len);

		char new_byte = (mask^bytes[byte_to_flip]);

		lseek(fd, start_range+byte_to_flip, SEEK_SET);
		write(fd, &new_byte, 1);
		deploy();

		lseek(fd, start_range+byte_to_flip, SEEK_SET);
		write(fd, &bytes[i], 1);

		i++;
	}



	lseek(fd, start_range, SEEK_SET);
	write(fd, bytes, len);
}


//This function tries all combinations of bits in a particular byte.
void bit_flip_in_byte(int fd, int byte_offset) {
	lseek(fd, byte_offset, SEEK_SET);
	unsigned char mask = 0;
	char byte = 0;
	read(fd, &byte, 1);
	lseek(fd, byte_offset, SEEK_SET);

	while (mask < 255) {
		// printf("%d", mask);
		char new_byte = (mask^byte);
		write(fd, &new_byte, 1);
		lseek(fd, byte_offset, SEEK_SET);
		mask++;
		deploy();
	}
	lseek(fd, byte_offset, SEEK_SET);
	write(fd, &byte, 1);

}

int file_length(int fd) {
	lseek(fd, 0, SEEK_SET);
	int file_len = lseek(fd,0,SEEK_END);
	return file_len;
}


int valid_num_char(char character) {
	int counter = 0;
	char valid_num_chars[] = "1234567890";
	while (counter <10) {
		if(character == valid_num_chars[counter]){
			return TRUE;
		}
		counter++;
	}
	return FALSE;
}


int number_end_offset(char* file_string, int num_start, int file_string_len) {
	
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
	lseek(fd, byte_offset, SEEK_SET);



	char* file_contents = malloc(file_len- byte_offset);
	read(fd, file_contents, file_len-byte_offset);

	// printf("%c\n",file_contents[0]);
	double number = 0;

	// printf("The file contents is: %s\n",file_contents);

	sscanf(file_contents,"%lf", &number);

	int num_length = number_end_offset(file_contents, 0, file_len-byte_offset);

	printf("The number length (characters): %d\n", num_length);

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
	

	lseek(fd, byte_offset, SEEK_SET);
	write(fd, file_contents, file_len-byte_offset);
	ftruncate(fd,file_len);
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





int main (int argc, char *argv[]) {
	// printf("%s\n",argv[1]);
	int fileDes = open("testdata.bin", O_RDWR);
	// printf("The bit mask is: \n");
	// printf("%d\n", fileDes);
	// bit_shift_in_range(fileDes, 0, 1);
	bit_flip_in_range(fileDes, 14, 5);
	// replace_numbers(fileDes, 14);
	// replace_strings(fileDes, 14, 21);




}

