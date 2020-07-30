#ifndef _FUZZ_JSON_H_
#define _FUZZ_JSON_H_

#include "fuzzer.h"

/* The handle for fuzzing json files, does not return */
void fuzz_handle_json(struct state *);

/* The handle for when the program exits */
void free_handle_json(struct state *);

#endif /* _FUZZ_JSON_H_ */

