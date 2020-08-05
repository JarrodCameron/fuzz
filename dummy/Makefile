# Author: Jarrod Cameron (z5210220)
# Date:   05/08/20 15:45

CC=gcc
CFLAGS=-Wall -Wextra -ggdb -fno-stack-protector -no-pie
BINS=dynamic32 dynamic64 static32 static64

all: $(BINS)

dynamic32:
	$(CC) dummy.c $(CFLAGS) -m32 -o dynamic32

dynamic64:
	$(CC) dummy.c $(CFLAGS) -o dynamic64

static32:
	$(CC) dummy.c $(CFLAGS) -static -m32 -o static32

static64:
	$(CC) dummy.c $(CFLAGS) -static -o static64

clean:
	rm -f $(BINS)
