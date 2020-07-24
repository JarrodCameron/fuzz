#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* File descriptors for the fuzzer to read/write to/from the fork server */
#define CMD_FD           198
#define INFO_FD          199

/* Helper functions */
static int run(void);

__attribute__ ((constructor))
void
shared(void)
{
	char cmd;
	int ret;

	printf("Ooooh we are inside the target\n");

	while (1) {

		ret = read(CMD_FD, &cmd, sizeof(cmd));
		assert(ret == sizeof(cmd));

		switch (cmd) {
		case 'R': /* run */
			if (run() == 0)
				return; /* Child proc returns */
			break;

		case 'Q': /* quit */
			exit(0);

		default:
			fprintf(stderr, "Unkown command: `%c` (%#hhx)", cmd, cmd);
			exit(1);
		}

	}
}

static
int
run(void)
{
	int ret, wstatus;

	/* Reset stdin */
	ret = lseek(0, 0, SEEK_SET);
	assert(ret >= 0);

	/* put the "fork" in "fork server" */
	ret = fork();
	assert(ret >= 0);

	if (ret == 0)
		return 0; /* child */

	ret = waitpid(ret, &wstatus, 0);
	assert(ret >= 0);

	ret = write(INFO_FD, &wstatus, sizeof(wstatus));
	assert(ret == sizeof(wstatus));

	return 1;
}
