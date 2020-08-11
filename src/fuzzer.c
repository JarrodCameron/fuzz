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
#include "fuzz_xml.h"
#include "fuzz_plaintext.h"
#include "fs.h"

/* Helper Functions */
static void fuzz_handle_dummy(struct state *);
static void sig_handler(UNUSED int sig);
static void init_state(const char *data, const char *bin, char **envp);
static int open_tmp_file(char **fname);

static struct state system_state = {0};

/* Called once and once only */
static void (*fuzz_handles[])(struct state *) = {
	[file_type_csv] = fuzz_handle_csv,
	[file_type_json] = fuzz_handle_json,
	[file_type_plain] = fuzz_handle_plaintext,
	[file_type_xml] = fuzz_handle_xml,
	[file_type_dummy] = fuzz_handle_dummy,
};

/* Called when the program is done, helps with checking for memory leaks */
static void (*free_handles[])(struct state *) = {
	[file_type_csv] = NULL,
	[file_type_json] = free_handle_json,
	[file_type_plain] = free_handle_plaintext,
	[file_type_xml] = free_handle_xml,
	[file_type_dummy] = NULL,
};

static
void
sig_handler(UNUSED int sig)
{
	/* We won't need this file anymore */
	sunlink(system_state.payload_fname);

	exit_fuzzer();
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

	system_state.payload_fd = open_tmp_file(&system_state.payload_fname);

	fs_init(&system_state);
}

NORETURN
void
exit_fuzzer(void)
{
	if (free_handles[system_state.ft] != NULL)
		free_handles[system_state.ft](&system_state);


	/* Signal that we are done */
	swrite(CMD_FD, CMD_QUIT, sizeof(CMD_QUIT)-1);

	free(system_state.payload_fname);

	exit(0);
}

static
int
open_tmp_file(char **fname)
{
	*fname = sstrdup(TESTDATA_FILE);
	return smkstemp(*fname);
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

	exit_fuzzer();
}

