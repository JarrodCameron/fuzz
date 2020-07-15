#ifndef _FUZZER_H_
#define _FUZZER_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ftype.h"

struct state {
	const char *input_file;
	struct stat stat;

	const char *binary;

	char **envp;

	char *mem;

	uint64_t deploys;

	/* The fd for the payload file.
	 * This saves use from opening/closing the file all the time. */
	int payload_fd;

	enum file_type ft;
};

#endif /* _FUZZER_H_ */

