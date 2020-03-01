/**
 * @author      : theo (theo.j.lincke@gmail.com)
 * @file        : error_macros
 * @created     : Wednesday Feb 26, 2020 20:14:26 MST
 */

#ifndef ERROR_MACROS_H

#define ERROR_MACROS_H

#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

FILE* file_open(const char *filename, const char *mode);
int file_close(FILE *fp);
int file_puts(const char* s, FILE *fp);
int file_gets(char *buf, int n, FILE* fp);
int seek_new_line(FILE* fp);
int file_exists(const char *f);

// Removes \n and \t from a buffer
int strip(char* s);

#endif /* end of include guard ERROR_MACROS_H */
