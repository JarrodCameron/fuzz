#include<stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//prototype 
// void bit_flip(int fd, int start_range, int end-range)


void deploy() {
	int fileDes = open("testdata.bin", O_RDONLY);
	// printf("%d\n", fileDes);
	// lseek(fileDes,0,SEEK_END);
	char buf[2] = {0};
	read(fileDes, buf, 2);
	printf("%c%c", buf[0],buf[1]);
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


int main (int argc, char *argv[]) {
	// printf("%s\n",argv[1]);
	int fileDes = open("testdata.bin", O_RDWR);
	// printf("The bit mask is: \n");
	// printf("%d\n", fileDes);
	bit_shift_in_range(fileDes, 0, 1);

}