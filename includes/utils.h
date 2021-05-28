#ifndef UTILS_H
#define UTILS_H

char *alloc_sprintf(const char *format, ...);
char *alloc_vsprintf(const char *format, va_list ap);

char *read_word(FILE *in);

#endif
