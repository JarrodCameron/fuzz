#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "fs.h"
#include "fuzz_plaintext.h"
#include "fuzzer.h"
#include "mutation_functions.h"
#include "safe.h"
#include "utils.h"




int count_lines(const char *file);




void
fuzz_handle_plaintext(struct state *state)
{
	int num_lines = count_lines(state->input_file);
	char ** lines = malloc(num_lines * sizeof(char*));
	char * token = strtok(state->mem, "\n");
	// loop through the string to extract all other tokens
	int i = 0;
	while( token != NULL ) {
		lines[i] = malloc(sizeof(char) * strlen(token+2));
		strcpy(lines[i],token);
		printf( ">>>>> %s\n", token ); //printing each token
		token = strtok(NULL, "\n");
		i++;
	}
	for(int j=0; j < num_lines; j++){
		lines[j][strlen(lines[j])] = '\n';
		lines[j][strlen(lines[j])+1] = '\0';
		printf("%s > %ld\n",lines[j],strlen(lines[j]));
		write(state->payload_fd, lines[j], strlen(lines[j]));
        }
	char buff[50];
	lseek(state->payload_fd,0,SEEK_SET);
	read(state->payload_fd, buff, 50);
	printf("%s",buff);
	int prev_bytes = 0;
	for(int j=0; j < num_lines; j++){
		printf("%s + %ld\n",lines[j],strlen(lines[j]));
		int i = 0;
		while(i < roll_dice(20,100)){
			bit_shift_in_range(state->payload_fd,prev_bytes,strlen(lines[j]));
			i++;
		}
		i = 0;
		while(i < roll_dice(20,100)){
			bit_flip_in_range(state->payload_fd,prev_bytes,strlen(lines[j]));
			i++;
		}

		if(lines[j][0] >= '0' && lines[j][0] <= '9'){
			printf(">>>>\n");
			printf("prev bytes: %d\n",prev_bytes);
			replace_numbers(state->payload_fd, prev_bytes);
		}
		else {
			replace_strings(state->payload_fd,prev_bytes,strlen(lines[j]));
	        }
		prev_bytes = prev_bytes + strlen(lines[j]);
	}
	// Fuzz each line separately
	// For each line: fuzz the line

	// Multi-line files imply different points of entry
	// Preserve first line to begin with
//	if(count_lines(state->input_file) > 1) {
//		char * first_line = strtok(state->mem, "\n");
//		write(state->payload_fd, first_line, strlen(first_line));
//		read(state->payload_fd,&first_line,10);
//		printf("%s\n",first_line);
//		while (1) {
//			uint32_t idx = roll_dice(0, ARRSIZE(fuzz_payloads_repeat)-1);
//			fuzz_payloads_repeat[idx](state);
//		}
//	}
}




int count_lines(const char *file){
	int line = 0;       // number of \n in the text
	FILE *fd = sfopen(file, "r");

	// finding all the special characters
	for (char c = getc(fd); c != EOF; c = getc(fd)) {
		switch(c){
			case '\n':
				line++;
		}
	}

	fclose(fd);
	return line;
}



