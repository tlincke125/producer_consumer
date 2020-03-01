#include "file-operations.h"

FILE* file_open(const char *filename, const char *mode) {
        FILE* ret = fopen(filename, mode);
        if(ret == NULL) {
                perror("Error no open\n");
        }
        return ret;
}

int file_close(FILE *fp) {
        int ret = fclose(fp);
        if(ret == EOF){
                perror("Error on close\n");
        }
        return ret;
}

int file_puts(const char* s, FILE *fp) {
        int ret = fputs(s, fp);
        if(ret == EOF) {
                perror("Error no puts\n");
        }
        return ret;
}


int file_gets(char *buf, int n, FILE* fp) {
        char* ret = fgets(buf, n, fp);
        if(ret == NULL) {
                return 0;
        }
        return 1;
}

int strip(char* s) {
        int removed = 0;
        char *p2 = s;

        while(*s != '\0') {
                if(*s != '\n') {
                        *p2++ = *s++;
                } else {
                        removed = 1;
                        ++s;
                }
        }
        *p2 = '\0';
        return removed;
}

int file_exists(const char *f)
{
    struct stat buffer;
    int exist = stat(f,&buffer);
    if(exist == 0)
        return 1;
    else // -1
        return 0;
}
