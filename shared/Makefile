CC=gcc
CFLAGS=-Wall -Wextra -ggdb -shared -fPIC -I$(INCDIR)

INCDIR=../include

BINS=shared32.so shared64.so

all: $(BINS)

shared32.so: shared.c
	$(CC) shared.c -o shared32.so $(CFLAGS) -m32

shared64.so: shared.c
	$(CC) shared.c -o shared64.so $(CFLAGS)

clean:
	rm -f $(BINS)

