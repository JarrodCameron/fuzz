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

/* Yanked these bad boys from:
 *     https://github.com/google/AFL/blob/master/config.h#L227
 */
UNUSED static const char *bad_num_strings[] = {
	"-128",          /* Overflow signed 8-bit when decremented  */
	"-1",            /*                                         */
	"0",             /*                                         */
	"1",             /*                                         */
	"16",            /* One-off with common buffer size         */
	"32",            /* One-off with common buffer size         */
	"64",            /* One-off with common buffer size         */
	"100",           /* One-off with common buffer size         */
	"127",           /* Overflow signed 8-bit when incremented  */
	"-32768",        /* Overflow signed 16-bit when decremented */
	"-129",          /* Overflow signed 8-bit                   */
	"128",           /* Overflow signed 8-bit                   */
	"255",           /* Overflow unsig 8-bit when incremented   */
	"256",           /* Overflow unsig 8-bit                    */
	"512",           /* One-off with common buffer size         */
	"1000",          /* One-off with common buffer size         */
	"1024",          /* One-off with common buffer size         */
	"4096",          /* One-off with common buffer size         */
	"32767",         /* Overflow signed 16-bit when incremented */
	"-2147483648",   /* Overflow signed 32-bit when decremented */
	"-100663046",    /* Large negative number (endian-agnostic) */
	"-32769",        /* Overflow signed 16-bit                  */
	"32768",         /* Overflow signed 16-bit                  */
	"65535",         /* Overflow unsig 16-bit when incremented  */
	"65536",         /* Overflow unsig 16 bit                   */
	"100663045",     /* Large positive number (endian-agnostic) */
	"2147483647",    /* Overflow signed 32-bit when incremented */
	"1337",          /* Adam's buffers are always 1337 bytes    */
};

/* Return true if string matches regex: `[+-]?[0-9][0-9]*` */
bool isint(const char *s, uint64_t len);

/* Generate a random number between `lo` and `hi` inclusive.
 * For example: roll_dice(2, 4) -> {2, 3, 4} */
uint32_t roll_dice(uint32_t lo, uint32_t hi);

/* When shit hits the fan, print this string and abort() */
NORETURN void panic(const char *fmt, ...);

#endif /* _UTILS_H_ */

