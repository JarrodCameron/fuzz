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
#include "ftype.h"
#include "fuzzer.h"

/* Helper Functions */
static void fuzz_handle_dummy(void);
static void sig_handler(UNUSED int sig);
static void init_state(const char *data, const char *bin, char **envp);
static void deploy(void);

static void (*fuzz_handles[])(void) = {
	[file_type_csv] = NULL,
	[file_type_json] = NULL,
	[file_type_plain] = NULL,
	[file_type_xml] = NULL,
	[file_type_dummy] = fuzz_handle_dummy,
};

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

	state.payload_fd = sopen(TESTDATA_FILE, O_RDWR | O_CREAT, 0644);
}

static
void
deploy(void)
{
	state.deploys += 1;
	int fd, wstatus;

	char *const argv[] = {
		(char *) "/bin/cat",
		NULL
	};

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

		swaitpid(pid, &wstatus, 0);

		if (WIFSIGNALED(wstatus) && WTERMSIG(wstatus) == SIGSEGV) {
			printf("We are done here\n");
			srename(TESTDATA_FILE, BAD_FILE);
			exit(0);
		}

		break;
	}

}

/* This function is a place holder, just an example of an example fuzzer
 * for a program i made up */
static
void
fuzz_handle_dummy(void)
{
	for (int i = 0; i < 256; i++) {
		slseek(state.payload_fd, 0, SEEK_SET);
		swrite(state.payload_fd, (void *) &i, sizeof(char));
		deploy();
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

	enum file_type ft = detect_file(state.mem, state.input_file);

	fuzz_handles[ft]();

	return 0;
}

