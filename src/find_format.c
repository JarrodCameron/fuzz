 #include <stdio.h>

 #define XML 1
 #define JSON 2
 #define CSV 3
 #define PLAINTEXT 4

// Have a separate main function to pass in these values
// for testing purposes, have an array with all the .txt files, and then loop through them and call this function
// If it gets all 10 correct then we're sweet
int find_format(char *filename) {
    int comma = 0;      // number of , in the text
    int curly_l = 0;    // number of { in the text
    int curly_r = 0;    // number of } in the text
    int angle_l = 0;    // number of < in the text
    int angle_r = 0;    // number of > in the text
    int line = 0;       // number of \n in the text

    FILE *fd = fopen(filename, "r");
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
        return JSON;
    } else if ((angle_l != 0) && (angle_l == angle_r) ) {
        printf("XML file found\n");
        return XML;
    } else if (comma == 0) {
        printf("plaintext file found\n");
        return PLAINTEXT;
    } else if (comma % line == 0) {
        printf("CSV file found\n");
        return CSV;
    } else {
        printf("plaintext file found\n");
        return PLAINTEXT;
    } 

    fclose(fd);
    return 0;
}

int main() {
    int file = find_format("csv1.txt");
    printf(file\n);

    return 0;
}