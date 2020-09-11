CC?=gcc
CFLAGS?=-Wall -ggdb -Wextra -I$(INCDIR) `xml2-config --cflags`

# TODO Make a logger
# TODO remove run.sh file
# TODO tidy up directory (we only need the final writeup)
# TODO remove -ggdb

INCDIR=include
SRCDIR=src
BUILDDIR=build
BINS=fuzzer
SHARED=shared32.so shared64.so
DUMMY=$(addprefix ./dummy/,dynamic32 dynamic64 static32 static64)
LIBS=-lcsv -ljsonparser -lm `xml2-config --libs`

SRC=$(shell ls $(SRCDIR))
OBJS=$(SRC:.c=.o)

all: $(DUMMY) $(BUILDDIR) $(SHARED) $(BINS)

fuzzer: $(addprefix $(BUILDDIR)/, $(OBJS))
	make -C libs
	$(CC) -o fuzzer $^ -Llibs $(LIBS)
	@echo ':)'

$(DUMMY): dummy/dummy.c
	make -C dummy

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
	make -C dummy clean
	rm -rf $(BUILDDIR) $(BINS) $(SHARED) vgcore.* bad.txt
