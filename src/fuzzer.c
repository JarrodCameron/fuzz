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
#include <time.h>

#include "safe.h"
#include "utils.h"
#include "config.h"
#include "mutation_functions.h"
#include "ftype.h"
#include "fuzzer.h"
#include "fuzz_csv.h"
#include "fuzz_json.h"

/* Helper Functions */
static void fuzz_handle_dummy(struct state *);
static void sig_handler(UNUSED int sig);
static void init_state(const char *data, const char *bin, char **envp);

static struct state system_state = {0};

static void (*fuzz_handles[])(struct state *) = {
	[file_type_csv] = fuzz_handle_csv,
	[file_type_json] = fuzz_handle_json,
	[file_type_plain] = NULL,
	[file_type_xml] = NULL,
	[file_type_dummy] = fuzz_handle_dummy,
};

static
void
sig_handler(UNUSED int sig)
{
	printf("Exiting: cya later\n");
	printf("# deploys: %lu\n", system_state.deploys);
	exit(0);
}

static
void
init_state(const char *data, const char *bin, char **envp)
{
	system_state.input_file = data;
	system_state.binary = bin;
	system_state.envp = envp;

	/* For graceful exit and some stats */
	ssignal(SIGINT, &sig_handler); /* Control-c */
	ssignal(SIGTERM, &sig_handler); /* timeout -v */

	int fd = sopen(system_state.input_file, O_RDONLY);

	sfstat(fd, &system_state.stat);

	system_state.mem = smmap(
		NULL,
		system_state.stat.st_size,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE,
		fd,
		0
	);

	sclose(fd);

	system_state.payload_fd = sopen(TESTDATA_FILE, O_RDWR | O_CREAT, 0644);
}

void
deploy(void)
{
	system_state.deploys += 1;
	int fd, wstatus;

	char *const argv[] = {
		(char *) system_state.binary,
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
			system_state.binary,
			argv,
			system_state.envp
		);

		break;

	default: /* Parent */

		swaitpid(pid, &wstatus, 0);

		if (WIFSIGNALED(wstatus) && WTERMSIG(wstatus) == SIGSEGV) {
			printf("$$$ SIGSEGV $$$\n");
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
fuzz_handle_dummy(UNUSED struct state *s)
{
	for (int i = 0; i < 256; i++) {
		slseek(system_state.payload_fd, 0, SEEK_SET);
		swrite(system_state.payload_fd, (void *) &i, sizeof(char));
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

	srand(time(NULL));

	init_state(argv[1], argv[2], envp);

	system_state.ft = detect_file(system_state.input_file);

	fuzz_handles[system_state.ft](&system_state);

	return 0;
}

