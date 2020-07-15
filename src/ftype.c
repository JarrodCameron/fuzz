#include "ftype.h"

enum file_type
detect_file(const char *mem, const char *file)
{
	/* TODO This is for @mich
	 * This should return the appropriate file type
	 * Not sure if "mem" and "file" is needed, removed if needed */
	(void) mem;
	(void) file;

	return file_type_csv;
}
