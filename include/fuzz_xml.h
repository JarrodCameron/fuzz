#ifndef _FUZZ_XML_H_
#define _FUZZ_XML_H_

#include "fuzzer.h"

/* The handle for fuzzing xml files, does not return */
void fuzz_handle_xml(struct state *);

/* The handle for when the program exits */
void free_handle_xml(struct state *);

#endif /* _FUZZ_XML_H_ */

