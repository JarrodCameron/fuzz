#ifndef CSV_DOT_H_INCLUDE_GUARD
#define CSV_DOT_H_INCLUDE_GUARD

#include <stdint.h>

#define CSV_ERR_LONGLINE 0
#define CSV_ERR_NO_MEMORY 1

//char **parse_csv( const char *line );
//void free_csv_line( char **parsed );
//char *fread_csv_line(FILE *fp, int max_line_size, int *done, int *err);

char **split_on_unescaped_newlines(const char *txt, uint64_t size);

#endif
