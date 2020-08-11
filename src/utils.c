#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "safe.h"
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

bool
isint0(const char *s)
{
	return isint(s, strlen(s));
}

uint64_t
roll_dice(uint64_t lo, uint64_t hi)
{

	return (rand() % (hi - lo + 1)) + lo;
}

uint32_t
coin_flip(uint32_t bias)
{
	return roll_dice(0, 99) < bias;
}

uint64_t
arr_len(const void **arr)
{
	uint64_t i = 0;
	while(arr[i])
		i++;
	return i;
}

void
move_file(const char *oldpath, const char *newpath)
{
	char buf[4096];
	ssize_t ret;

	int oldfd = sopen(oldpath, O_RDONLY);
	int newfd = sopen(newpath, O_WRONLY | O_CREAT, 0644);

	/* Empty the file in case there is anything inside of it */
	sftruncate(newfd, 0);

	while ((ret = sread(oldfd, buf, ARRSIZE(buf))) > 0)
		/* we should loop here for partial writes but ain't nobody got time
		 * for that */
		swrite(newfd, buf, ret);

	sclose(oldfd);
	sclose(newfd);
}

static
bool
char_is_num(char c)
{
	return ('0' <= c) && (c <= '9');
}
