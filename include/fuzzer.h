#ifndef _FUZZER_H_
#define _FUZZER_H_

static struct {
	const char *input_file;
	struct stat stat;

	const char *binary;

	char **envp;

	char *mem;

	uint64_t deploys;

	/* The fd for the payload file.
	 * This saves use from opening/closing the file all the time. */
	int payload_fd;
} state = {0};

#endif /* _FUZZER_H_ */

