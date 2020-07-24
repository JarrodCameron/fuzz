#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdbool.h>
#include <stdint.h>

#define BIG "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" \
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" \
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" \
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" \
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" \
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" \
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" \
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" \
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" \
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" \
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" \
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" \
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" \
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" \
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" \
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"

#define UNUSED __attribute__((unused))
#define NORETURN __attribute__((noreturn))
#define BP() __asm__("int3")
#define ARRSIZE(A) (sizeof(A)/sizeof(A[0]))
#define MIN(A,B) ((A<B)?(A):(B))
#define MAX(A,B) ((A>B)?(A):(B))

/* Yanked these bad boys from:
 *     https://github.com/google/AFL/blob/master/config.h#L227
 */
UNUSED static struct {
	int64_t n;
	const char *s;
} bad_nums[] = {
	{-128,        "-128"},        /* Overflow signed 8-bit when decremented  */
	{-1,          "-1"},          /*                                         */
	{0,           "0"},           /*                                         */
	{1,           "1"},           /*                                         */
	{16,          "16"},          /* One-off with common buffer size         */
	{32,          "32"},          /* One-off with common buffer size         */
	{64,          "64"},          /* One-off with common buffer size         */
	{100,         "100"},         /* One-off with common buffer size         */
	{127,         "127"},         /* Overflow signed 8-bit when incremented  */
	{-32768,      "-32768"},      /* Overflow signed 16-bit when decremented */
	{-129,        "-129"},        /* Overflow signed 8-bit                   */
	{128,         "128"},         /* Overflow signed 8-bit                   */
	{255,         "255"},         /* Overflow unsig 8-bit when incremented   */
	{256,         "256"},         /* Overflow unsig 8-bit                    */
	{512,         "512"},         /* One-off with common buffer size         */
	{1000,        "1000"},        /* One-off with common buffer size         */
	{1024,        "1024"},        /* One-off with common buffer size         */
	{4096,        "4096"},        /* One-off with common buffer size         */
	{32767,       "32767"},       /* Overflow signed 16-bit when incremented */
	{-2147483648, "-2147483648"}, /* Overflow signed 32-bit when decremented */
	{-100663046,  "-100663046"},  /* Large negative number (endian-agnostic) */
	{-32769,      "-32769"},      /* Overflow signed 16-bit                  */
	{32768,       "32768"},       /* Overflow signed 16-bit                  */
	{65535,       "65535"},       /* Overflow unsig 16-bit when incremented  */
	{65536,       "65536"},       /* Overflow unsig 16 bit                   */
	{100663045,   "100663045"},   /* Large positive number (endian-agnostic) */
	{2147483647,  "2147483647"},  /* Overflow signed 32-bit when incremented */
	{1337,        "1337"},        /* Adam's buffers are always 1337 bytes    */
};

/* List of format string which will probably segfault if victim is
 * vulnerable */
UNUSED static const char *fmt_strings[] = {
	"%1$s", "%2$s", "%3$s", "%4$s", "%5$s", "%6$s", "%7$s", "%8$s", "%9$s"
};

/* Return true if string matches regex: `[+-]?[0-9][0-9]*` */
bool isint(const char *s, uint64_t len);

/* Generate a random number between `lo` and `hi` inclusive.
 * For example: roll_dice(2, 4) -> {2, 3, 4} */
uint32_t roll_dice(uint32_t lo, uint32_t hi);

/* Flip a bias coin. The bias is between zero and 100. This is done by
 * generating a random number. If the rand number is less than the bias
 * return 1, else return 0. */
uint32_t coin_flip(uint32_t bias);

/* Return the number of items in the array. This function assumes the last
 * element in the array is NULL. */
uint64_t arr_len(const void **arr);

/* When shit hits the fan, print this string and abort() */
NORETURN void panic(const char *fmt, ...);

#endif /* _UTILS_H_ */

