/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   10/07/20 18:46               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>

#include "safe.h"
#include "utils.h"
#include "config.h"
#include "fuzzer.h"
#include "mutation_functions.h"

struct {
	const char *input_file;
	struct stat stat;

	const char *binary;

	char **envp;

	char *mem;

	uint64_t deploys;
} state = {0};

static
void
sig_handler(UNUSED int sig)
{
	printf("cya later\n");
	printf("%lu\n", state.deploys);
	exit(0);
}

static
void
init_state(const char *data, const char *bin, char **envp)
{
	state.input_file = data;
	state.binary = bin;
	state.envp = envp;

	/* For graceful exit and some stats */
	ssignal(SIGINT, &sig_handler); /* Control-c */
	ssignal(SIGTERM, &sig_handler); /* timeout -v */

	int fd = sopen(state.input_file, O_RDONLY);

	sfstat(fd, &state.stat);

	state.mem = smmap(
		NULL,
		state.stat.st_size,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE,
		fd,
		0
	);

	sclose(fd);
}

/* Dump the contents of the mmap'd area onto disk */
static
void
dump_mem(void)
{
	int fd = sopen(TESTDATA_FILE, O_WRONLY);
	swrite(fd, state.mem, state.stat.st_size);
	sclose(fd);
}

void
deploy(void)
{
	state.deploys += 1;
	int fd, wstatus;

	char *const argv[] = {
		(char *) state.binary,
		NULL
	};

	dump_mem();

	pid_t pid = sfork();

	switch (pid) {
	case 0: /* Child */

		fd = sopen(TESTDATA_FILE, O_RDONLY);

		/* Overwrite stdin in the child process */
		sdup2(fd, 0);

		/* won't need this anymore */
		sclose(fd);

		sexecve(
			state.binary,
			argv,
			state.envp
		);

		break;

	default: /* Parent */

		waitpid(pid, &wstatus, 0);
		break;
	}

}

int
main(int argc, char **argv, char **envp)
{

	if (argc != 3) {
		fprintf(stderr, "Usage: %s /path/to/data /path/to/bin\n", argv[0]);
		exit(1);
	}

	init_state(argv[1], argv[2], envp);


	while(1)
		// deploy();

	return 0;
}

