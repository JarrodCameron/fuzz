#ifndef _FUZZER_H_
#define _FUZZER_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#include "ftype.h"
#include "utils.h"

struct state {
	const char *input_file;
	struct stat stat;

	const char *binary;

	char **envp;

	char *mem;

	/* The fd for the payload file.
	 * This saves use from opening/closing the file all the time. */
	int payload_fd;

	/* Name of the file, this is only used to prevent leaks */
	char *payload_fname;

	enum file_type ft;
};

/* This function is like a C++ deconstructor, we call this when we are done
 * with the fuzzer, whether we found a SIGSEGV or not. */
NORETURN void exit_fuzzer(void);

#endif /* _FUZZER_H_ */

