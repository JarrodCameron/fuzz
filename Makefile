# Author: Jarrod Cameron (z5210220)
# Date:   10/07/20 21:12

CC=gcc
CFLAGS=-Wall -Wextra -ggdb -Werror -I$(INCDIR)

INCDIR=include
SRCDIR=src
BUILDDIR=build
BINS=fuzzer

DEPS= \
	safe.o \
	utils.o \
	fuzzer.o

all: $(BUILDDIR) $(BINS)

fuzzer: $(addprefix $(BUILDDIR)/, $(DEPS))
	$(CC) -o fuzzer $(addprefix $(BUILDDIR)/, $(DEPS))

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILDDIR) $(BINS) testdata.bin

