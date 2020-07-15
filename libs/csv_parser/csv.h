#ifndef CSV_DOT_H_INCLUDE_GUARD
#define CSV_DOT_H_INCLUDE_GUARD

#include <stdint.h>
#include <stdio.h>

#define CSV_ERR_LONGLINE 0
#define CSV_ERR_NO_MEMORY 1

/* Given a line of the csv file, return array of each value.
 * Array is null terminated */
char **parse_csv( const char *line );

void free_csv_line( char **parsed );
char *fread_csv_line(FILE *fp, int max_line_size, int *done, int *err);
char **split_on_unescaped_newlines(const char *txt, uint64_t size);

#endif
