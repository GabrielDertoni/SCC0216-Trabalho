#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

char *alloc_vsprintf(const char *format, va_list ap) {
    va_list ap_cpy;
    va_copy(ap_cpy, ap);

    int n_chars = vsnprintf(NULL, 0, format, ap_cpy);
    char *s = (char *)malloc((n_chars + 1) * sizeof(char));

    int n_written = vsnprintf(s, n_chars + 1, format, ap);

    // Something terrible has happened!
    if (n_written != n_chars) {
        free(s);
        s = NULL;
    }

    va_end(ap_cpy);
    return s;
}

char *alloc_sprintf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);

    char *s = alloc_vsprintf(format, ap);

    va_end(ap);
    return s;
}
