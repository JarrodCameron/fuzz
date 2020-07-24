#ifndef _CONFIG_H_
#define _CONFIG_H_

/* Here lies random config data, changing these value(s) SHOULD not stuff up
 * the program */

/* TODO This should be a file in /tmp */
#define TESTDATA_FILE "testdata.bin"

/* File descriptors for the fuzzer to read/write to/from the fork server */
#define CMD_FD           198
#define INFO_FD          199

/* Commands for the fork server */
#define CMD_RUN  "R"
#define CMD_QUIT "Q"

#define TRUE 1
#define FALSE 0

#define NUM_OF_BIT_FLIP_ITERATIONS_CONSTANT 4
#define NUM_OF_BIT_SHIFT_ITERATIONS_CONSTANT 4
#define BAD_FILE "bad.txt"


#endif /* _CONFIG_H_ */

