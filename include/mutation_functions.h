#ifndef _MUTATION_FUNC_H_
#define _MUTATION_FUNC_H_

#include <stdint.h>

//args
// fd: file descriptor
// start_range: the start index of the bits you want to shift
// len: number of bytes you wish to shift e.g. will shift bytes from start_range:start_range+len
void bit_shift_in_range(int fd, uint32_t start_range, uint32_t len);

//args
// fd: file descriptor
// start_range: the start index of the bits you want to flip
// len: number of bytes you wish to flip e.g. will flip bytes from start_range:start_range+len

void bit_flip_in_range(int fd, int start_range, int len);
//args
// fd: file descriptor
// byte_offset: the location to the start of the number you wish to replace. The number could be int or float
void replace_numbers(int fd, int byte_offset);

//args
// fd: file descriptor
// byte_offset: the start index of the string you wish to replace
// replace_str_len: the length of the string you wish to replace
void replace_strings(int fd, int byte_offset, int replace_str_len);

#endif /* _MUTATION_FUNC_H_ */
