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

#endif /* _FS_H_ */

