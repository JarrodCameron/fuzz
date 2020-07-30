#include "ftype.h"
#include "utils.h"
#include "safe.h"

/* This should return the appropriate file type */
enum file_type
detect_file(const char *file)
{
	int comma = 0;      // number of , in the text
	int curly_l = 0;    // number of { in the text
	int curly_r = 0;    // number of } in the text
	int angle_l = 0;    // number of < in the text
	int angle_r = 0;    // number of > in the text
	int line = 0;       // number of \n in the text

	FILE *fd = sfopen(file, "r");

	// finding all the special characters
	for (char c = getc(fd); c != EOF; c = getc(fd)) {
		switch(c){
			case ',':
				comma++;
				break;
			case '{':
				curly_l++;
				break;
			case '}':
				curly_r++;
				break;
			case '<':
				angle_l++;
				break;
			case '>':
				angle_r++;
				break;
			case '\n':  // should I add '\0' as well?
				line++;
				break;
		}
	}

	fclose(fd);

	if ((curly_l != 0) && (curly_l == curly_r) )
		return file_type_json;

	if ((angle_l != 0) && (angle_l == angle_r) )
		return file_type_xml;

	if (comma == 0)
		return file_type_plain;

	if (comma % line == 0)
		return file_type_csv;

	return file_type_plain;
}

/* Return the name of the file_type, good for debugging */
const char *
dbg_file_type(enum file_type ft)
{
	switch (ft) {
		case file_type_csv: return "file_type_csv";
		case file_type_json: return "file_type_json";
		case file_type_plain: return "file_type_plain";
		case file_type_xml: return "file_type_xml";
		case file_type_dummy: return "file_type_dummy";
	}
	panic("wtf");
}
