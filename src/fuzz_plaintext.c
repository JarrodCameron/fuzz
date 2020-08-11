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
		token = strtok(NULL, "\n");
		i++;
	}
	int k = 0;
	for(int j=0; j < num_lines; j++){
		lines[j][strlen(lines[j])] = '\n';
		lines[j][strlen(lines[j])+1] = '\0';
		write(state->payload_fd, lines[j], strlen(lines[j]));
        }
	int prev_bytes = 0;
	for(int j=0; j < num_lines; j++){
		if(lines[j][0] >= '0' && lines[j][0] <= '9'){
			replace_numbers(state->payload_fd, prev_bytes);
		}
		else {
			replace_strings(state->payload_fd,prev_bytes,strlen(lines[j]));
	        }
		prev_bytes = prev_bytes + strlen(lines[j]);
	}
	while(1) {
		prev_bytes = 0;
		for(int j=0; j < num_lines; j++){
			if(num_lines <= 1){
				while(1){
					bit_shift_in_range(state->payload_fd,prev_bytes,strlen(lines[j]));
					bit_flip_in_range(state->payload_fd,prev_bytes,strlen(lines[j]));
				}
			}
			else {
				int i =0;
				while(i < roll_dice(num_lines,1337)) {
					bit_shift_in_range(state->payload_fd,prev_bytes,strlen(lines[j]));
					bit_flip_in_range(state->payload_fd,prev_bytes,strlen(lines[j]));
					i++;
				}	
			}
			prev_bytes = prev_bytes + strlen(lines[j]);
		}
	}
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



