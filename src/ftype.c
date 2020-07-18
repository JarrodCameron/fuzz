#include <stdio.h>

#include "ftype.h"
#include "utils.h"

enum file_type
detect_file(const char *file)
{
	// This should return the appropriate file type

    int comma = 0;      // number of , in the text
    int curly_l = 0;    // number of { in the text
    int curly_r = 0;    // number of } in the text
    int angle_l = 0;    // number of < in the text
    int angle_r = 0;    // number of > in the text
    int line = 0;       // number of \n in the text

    FILE *fd = fopen(file, "r");
    if (fd == NULL) {
        printf("Could not open file %s", file);
        return 0;
    }

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
            default:
                break;
        }
    }

    // printf("%d comma, %d {, %d }, %d <, %d >, %d lines\n", comma, curly_l, curly_r, angle_l, angle_r, line);

    if ((curly_l != 0) && (curly_l == curly_r) ) {
        // printf("JSON file found\n");
        return file_type_json;
    } else if ((angle_l != 0) && (angle_l == angle_r) ) {
        // printf("XML file found\n");
        return file_type_xml;
    } else if (comma == 0) {
        // printf("plaintext file found\n");
        return file_type_plain;
    } else if (comma % line == 0) {
        // printf("CSV file found\n");
        return file_type_csv;
    } else {
        // printf("plaintext file found\n");
        return file_type_plain;
    }

    fclose(fd);
	// shouldn't ever get here
    return file_type_dummy;
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
