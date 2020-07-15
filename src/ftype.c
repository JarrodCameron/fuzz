#include "ftype.h"

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
        printf("Could not open file %s", filename);
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
    line++; // for the final line
    printf("%d comma, %d {, %d }, %d <, %d >, %d lines\n", comma, curly_l, curly_r, angle_l, angle_r, line);

    if ((curly_l != 0) && (curly_l == curly_r) ) {
        printf("JSON file found\n");
        return file_type_json;
    } else if ((angle_l != 0) && (angle_l == angle_r) ) {
        printf("XML file found\n");
        return file_type_xml;
    } else if (comma == 0) {
        printf("plaintext file found\n");
        return file_type_plain;
    } else if (comma % line == 0) {
        printf("CSV file found\n");
        return file_type_csv;
    } else {
        printf("plaintext file found\n");
        return file_type_plain;
    } 

    fclose(fd);
    return file_type_dummy;
}
