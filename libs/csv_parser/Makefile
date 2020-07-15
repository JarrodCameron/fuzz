LIB=libcsv.a

all:
	gcc -c -fpic *.c
	ar rcs $(LIB) *.o
	cp $(LIB) ../

foo:

clean:
	rm -f *.a *.o

# From the source
all_:
	gcc -g -Wall csv.c split.c fread_csv_line.c tests/test.c -o test
