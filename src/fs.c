#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"
#include "display.h"
#include "fs.h"
#include "safe.h"
#include "utils.h"

/* Helper functions */
static void spawn_target(struct state *s);
static unsigned char get_elf_class(struct state *s);
static void **arr_join(const void **a, const void **b);
static void parent_pipes_init(int cmd_pipe[2], int info_pipe[2]);
static void child_pipes_init(int cmd_pipe[2], int info_pipe[2]);
static int boring_deploy(void);
static int fs_test(void);
static void set_target_output(void);

/* Using for overwriting the deploy function */
static int (*deploy_hook)(void) = NULL;

static struct state *system_state = NULL;

void
fs_init(struct state *s)
{
	int cmd_pipe[2] = {0}; /* Fuzzer reads this pipe */
	int info_pipe[2] = {0}; /* Fuzzer writes this pipe */

	/* saved for later use */
	system_state = s;

	spipe(cmd_pipe);
	spipe(info_pipe);

	pid_t pid = sfork();

	switch (pid) {
	case 0: /* child */
		child_pipes_init(cmd_pipe, info_pipe);
		set_target_output();
		spawn_target(s);

	default: /* parent */
		parent_pipes_init(cmd_pipe, info_pipe);
		if (fs_test() < 0)
			deploy_hook = &boring_deploy;
		break;
	}
}

int
deploy(void)
{
	int wstatus;

	display_deploy();

	if (deploy_hook)
		return deploy_hook();

	/* Tell the fork server to run */
	swrite(CMD_FD, CMD_RUN, sizeof(CMD_RUN)-1);

	/* Get the result of waitpid() from the fork server */
	sread(INFO_FD, &wstatus, sizeof(wstatus));

	if (WIFSIGNALED(wstatus) && WTERMSIG(wstatus) == SIGSEGV) {

		move_file(system_state->payload_fname, BAD_FILE);

		exit_fuzzer();
	}

	return wstatus;
}

static
void
set_target_output(void)
{
#ifdef TARGET_OUTPUT
	int fd = sopen(TARGET_OUTPUT, O_WRONLY);

	sdup2(fd, 1 /* stdin */);
	sdup2(fd, 2 /* stderr */);

	sclose(fd);
#else
	return;
#endif /* TARGET_OUTPUT */

}

static
int
boring_deploy(void)
{
	int wstatus, fd;

	char *const argv[] = {
		(char *) system_state->binary,
		NULL
	};

	pid_t pid = sfork();

	switch (pid) {
	case 0: /* Child */

		fd = sopen(system_state->payload_fname, O_RDONLY);

		/* Overwrite stdin in the child process */
		sdup2(fd, 0);

		/* won't need this anymore */
		sclose(fd);

		sexecve(
			system_state->binary,
			argv,
			system_state->envp
		);

		panic("We should never get here D:\n");

	default: /* Parent */
		swaitpid(pid, &wstatus, 0);

		if (WIFSIGNALED(wstatus) && WTERMSIG(wstatus) == SIGSEGV) {

			move_file(system_state->payload_fname, BAD_FILE);

			exit_fuzzer();
		}
		break;
	}

	return wstatus;
}

static
int
fs_test (void)
{
	ssize_t ret;
	char buf[4] = {0};

	ret = write(CMD_FD, CMD_TEST, sizeof(CMD_TEST)-1);
	if (ret < 0)
		return -1;

	ret = write(CMD_FD, "SYN", 3);
	if (ret < 0)
		return -1;

	ret = read(INFO_FD, &buf, 3);
	if (ret < 0)
		return -1;

	if (strcmp(buf, "ACK") != 0)
		return -1;

	return 0;
}

/* The child (aka the target) needs to:
 * - read from the cmd pipe (since the fuzzer gives commands)
 * - write to the info pipe (so the fuzzer can parse results)
 */
static
void
child_pipes_init(int cmd_pipe[2], int info_pipe[2])
{
	sclose(cmd_pipe[1]); /* read endpoint */
	sclose(info_pipe[0]); /* write endpoint */

	sdup2(cmd_pipe[0], CMD_FD);
	sdup2(info_pipe[1], INFO_FD);
}

/* The parent (aka the fuzzer) needs to:
 * - write to the cmd pipe (to tell the fork server what to do)
 * - read from the info pipe (to parse results of the fork server)
 */
static
void
parent_pipes_init(int cmd_pipe[2], int info_pipe[2])
{
	sclose(cmd_pipe[0]); /* write endpoint */
	sclose(info_pipe[1]); /* read endpoint */

	sdup2(cmd_pipe[1], CMD_FD);
	sdup2(info_pipe[0], INFO_FD);
}

/* Where the child process starts, will start the target process */
NORETURN
static
void
spawn_target(struct state *s)
{
	unsigned char elf_class = get_elf_class(s);

	char *const argv[] = {
		(char *) s->binary,
		NULL
	};

	const char *new_env[] = {0};

	if (elf_class == ELFCLASS32) {
		/* Load our custom 32bit library */
		new_env[0] = "LD_PRELOAD=./shared32.so";
	} else if (elf_class == ELFCLASS64) {
		/* Load our custom 64bit library */
		new_env[0] = "LD_PRELOAD=./shared64.so";
	} else {
		panic("Unkown elf class, fix your elf file");
	}

	/* Solve all symbols (i.e. the GOT) before loading fork server */
	new_env[1] = "LD_BIND_NOW=1";

	/* Overwrite standard input with our input file */
	int fd = sopen(s->payload_fname, O_RDONLY);
	sdup2(fd, 0);
	sclose(fd);

	sexecve(
		s->binary,
		argv,
		(void *) arr_join(
			(void *) s->envp,
			(void *) new_env
		) /* Data types are hard :( */
	);

}

/* Given two NULL terminated arrays, join them together */
static
void **
arr_join(const void **a, const void **b)
{
	uint64_t alen = arr_len(a);
	uint64_t blen = arr_len(b);
	uint64_t len = alen + blen + 1;
	void **ret = smalloc(sizeof(void *) * len);

	memcpy(&ret[0], a, alen * sizeof(void *));
	memcpy(&ret[alen], b, blen * sizeof(void *));
	ret[alen + blen] = NULL;

	return ret;

}

/* Return the elf class
 * For noobs: Returns if the elf file is 32 or 64 bits */
static
unsigned char
get_elf_class(struct state *s)
{
	unsigned char e_ident[EI_NIDENT];

	int fd = sopen(s->binary, O_RDONLY);
	sread(fd, e_ident, ARRSIZE(e_ident));
	sclose(fd);
	return e_ident[EI_CLASS];
}
