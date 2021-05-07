#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

char *alloc_sprintf(const char *format, ...) {
    va_list ap;
    va_list ap_cpy;
    va_start(ap, format);
    va_copy(ap_cpy, ap);

    int n_chars = vsnprintf(NULL, 0, format, ap_cpy);
    char *s = (char *)malloc(n_chars * sizeof(char));

    int n_written = vsnprintf(s, n_chars, format, ap);

    // Something terrible has happened!
    if (n_written != n_chars) {
        free(s);
        return NULL;
    }

    va_end(ap);
    va_end(ap_cpy);
    return s;
}
