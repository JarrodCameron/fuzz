#ifndef _FUZZ_PLAINTEXT_H_
#define _FUZZ_PLAINTEXT_H_

#include "fuzzer.h"

/* The handle for fuzzing plaintext files, does not return */
void fuzz_handle_plaintext(struct state *);

/* The handle for freeing up all data structures used by plain text fuzzer */
void free_handle_plaintext(struct state *s);

#endif /* _FUZZ_PLAINTEXT_H_ */

