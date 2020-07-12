#include<stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>


#define TRUE 1
#define FALSE 0

//prototype 
// void bit_flip(int fd, int start_range, int end-range)


//Issues
//Currently leaves 0s at the end of the file if the length of the file has been reduced with replace numbers
//Need a different strategy for bit flipping at 255^num bits is 60 seconds for flipping 3 bytes, duplicates also appear

void deploy() {
	int fileDes = open("testdata.bin", O_RDONLY);
	// printf("%d\n", fileDes);
	// lseek(fileDes,0,SEEK_END);
	char buf[81] = {0};
	read(fileDes, buf, 80);
	printf("%s\n", buf);
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


void bit_flip_in_range(int fd, int start_range, int end_range) {

	if(start_range > end_range) {
		deploy();
		return;
	}
	lseek(fd, start_range, SEEK_SET);
	unsigned char mask = 0;
	char byte = 0;
	read(fd, &byte, 1);
	lseek(fd, start_range, SEEK_SET);


	int iterations = 0;
	while (iterations < 256) {
		// printf("%d, ", mask);
		char new_byte = (mask^byte);
		// printf("The range is %d - %d\n", start_range, end_range);
		// if(new_byte == 0) {
		// 	printf("new_byte is 0\n");
		// }
		lseek(fd, start_range, SEEK_SET);
		write(fd, &new_byte, 1);
		bit_flip_in_range(fd, start_range+1, end_range);
		mask++;
		iterations++;
	}

	lseek(fd, start_range, SEEK_SET);
	write(fd, &byte, 1);
	// deploy()

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
	char * quick_string = calloc((new_number_length+1),sizeof(char));
	snprintf( quick_string, new_number_length+1, "%ld", number);
	// printf("Quick string is: %s\n", quick_string);
	write(fd, quick_string, new_number_length); //Does not include null terminator
	// printf("String is: %s\n",&file_contents[num_length]);
	write(fd, &file_contents[num_length], file_len-byte_offset-num_length);
	char * zeros = calloc(sizeof(char),file_len- byte_offset);
	write(fd, zeros, file_len- byte_offset);
	free(zeros);
	
	deploy();
}

// byte_offset, the offset of the file for which the number it is, it can be a float or integer
void replace_numbers(int fd, int byte_offset) {
	int file_len = file_length(fd);
	lseek(fd, byte_offset, SEEK_SET);



	char* file_contents = malloc(file_len- byte_offset);
	read(fd, file_contents, file_len-byte_offset);

	double number = 0;

	// printf("The file contents is: %s\n",file_contents);

	sscanf(file_contents,"%lf", &number);

	int num_length = number_end_offset(file_contents, 0, file_len-byte_offset);

	// printf("The number length (characters): %d\n", num_length);

	// printf("%lf\n", number);

	write_int_number(fd, byte_offset, file_contents, num_length, 0);
	write_int_number(fd, byte_offset, file_contents, num_length, -2);

	write_int_number(fd, byte_offset, file_contents, num_length, 100);
	
	lseek(fd, byte_offset, SEEK_SET);
	write(fd, file_contents, file_len-byte_offset);
	char * zeros = calloc(sizeof(char),file_len- byte_offset);
	write(fd, zeros, file_len- byte_offset);
	free(zeros);

}




int main (int argc, char *argv[]) {
	// printf("%s\n",argv[1]);
	int fileDes = open("testdata.bin", O_RDWR);
	// printf("The bit mask is: \n");
	// printf("%d\n", fileDes);
	// bit_shift_in_range(fileDes, 0, 1);
	replace_numbers(fileDes, 18);



}

