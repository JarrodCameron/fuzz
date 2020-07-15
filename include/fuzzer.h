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

/* A payload that has been written to the file TESTDATA_FILE will be used as
 * input to the victim binary. If we find a bug, we do not return. */
void deploy(void);

#endif /* _FUZZER_H_ */

