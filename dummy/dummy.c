#include <assert.h>
#include <stdio.h>
#include <unistd.h>

int main (void)
{
	printf("Hello, I am a bad program\n");
	printf("Send me an \"A\" and I will segfault\n");

	char buf[4096] = {0};
	int ret = read(0, buf, 4096);
	assert(ret >= 0);
	for (int i = 0; i < 4096; i++) {
		if (buf[i] == 'A') {
			char *p = (void *) 0xdeadbeef;
			*p = 0; /* segfault */
		}
	}
	return 0;
}

