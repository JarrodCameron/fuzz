# Author: Jarrod Cameron (z5210220)
# Date:   10/07/20 21:12

CC=gcc
CFLAGS=-Wall -ggdb -Wextra -I$(INCDIR)

INCDIR=include
SRCDIR=src
BUILDDIR=build
BINS=fuzzer

SRC=$(shell ls $(SRCDIR))
OBJS=$(SRC:.c=.o)

all: $(BUILDDIR) $(BINS)

fuzzer: $(addprefix $(BUILDDIR)/, $(OBJS))
	make -C libs
	$(CC) -static -o fuzzer $^ -Llibs -lcsv -ljsonparser -lm
	@echo ':)'

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	make -C libs clean
	rm -rf $(BUILDDIR) $(BINS) testdata.bin
