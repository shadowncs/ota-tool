#ifndef _H_UTIL
#define _H_UTIL 1

#include <stdio.h>

#define malloc_t(type, count) (type*)malloc(sizeof(type) * (count))

int open_img_file(char* dir, const char* file, int flags);

char* read_alloc(FILE *f, int size);

#endif /* ifndef _H_UTIL */
