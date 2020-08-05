# Author: Jarrod Cameron (z5210220)
# Date:   10/07/20 21:12

CC=gcc
CFLAGS=-Wall -ggdb -Wextra -I$(INCDIR) `xml2-config --cflags`

INCDIR=include
SRCDIR=src
BUILDDIR=build
BINS=fuzzer
SHARED=shared32.so shared64.so
LIBS=-lcsv -ljsonparser -lm `xml2-config --libs`

SRC=$(shell ls $(SRCDIR))
OBJS=$(SRC:.c=.o)

all: $(BUILDDIR) $(SHARED) $(BINS)

fuzzer: $(addprefix $(BUILDDIR)/, $(OBJS))
	make -C libs
	$(CC) -o fuzzer $^ -Llibs $(LIBS)
	@echo ':)'

$(SHARED): shared/shared.c
	make -C shared $@
	cp shared/$@ .

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	make -C libs clean
	make -C shared clean
	rm -rf $(BUILDDIR) $(BINS) testdata.bin $(SHARED) vgcore.* bad.txt
