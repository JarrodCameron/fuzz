#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "utils.h"

/* Helper Functions */
static bool char_is_num(char c);

void
panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	abort();
}

bool
isint(const char *s, uint64_t len)
{
	uint64_t sofar = 0;

	if (len == 0)
		return false;

	if (s[0] == '+' || s[0] == '-')
		sofar += 1;

	if (!char_is_num(s[sofar]))
		return false;

	for (; sofar < len; sofar++) {
		if (!char_is_num(s[sofar]))
			return false;
	}

	return true;

}

uint32_t
roll_dice(uint32_t lo, uint32_t hi)
{
	return (rand() % (hi - lo + 1)) + lo;
}

static
bool
char_is_num(char c)
{
	return ('0' <= c) && (c <= '9');
}
