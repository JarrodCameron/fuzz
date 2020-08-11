#ifndef _FS_H_
#define _FS_H_

/* The wrappers around the fork server. This was inspired from the following
 * article:
 *
 * https://lcamtuf.blogspot.com/2014/10/fuzzing-binaries-without-execve.html
 *
 * tldr:
 * - LD_PRELOAD lets us load our custom library which will act as a fork
 *   server. It listens for commands from our fuzzer.
 * - LD_BIND_NOW will link all of the references from the GOT before invoking
 *   our shared library
 *
 * This results in about a 2x speed up :)
 */

#include "fuzzer.h"

/* Initialise the fork server for the client */
void fs_init(struct state *);

/* A payload that has been written to the file TESTDATA_FILE will be used as
 * input to the victim binary. If we get a sigsegv, we do not return.
 *
 * We return the result of "wstatus" from "waitpid"
 */
int deploy(void);

#endif /* _FS_H_ */

