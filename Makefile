# Author: Jarrod Cameron (z5210220)
# Date:   10/07/20 21:12

CC=gcc
CFLAGS=-Wall -ggdb -Wextra -I$(INCDIR)

INCDIR=include
SRCDIR=src
BUILDDIR=build
BINS=fuzzer

DEPS= \
	safe.o \
	utils.o \
	fuzzer.o \
	fuzz_csv.o \
	ftype.o

all: $(BUILDDIR) $(BINS)

fuzzer: $(addprefix $(BUILDDIR)/, $(DEPS))
	make -C libs
	$(CC) -static -o fuzzer $(addprefix $(BUILDDIR)/, $(DEPS)) -Llibs -lcsv

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILDDIR) $(BINS) testdata.bin
	make -C libs clean
