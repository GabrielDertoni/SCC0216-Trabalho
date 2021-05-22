#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define QUOTE 34
#define SPACE 32
#define BREAK_LINE 10
#define CARRIAGE_RETURN 13
#define and &&

#define BUFFER 256

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

char *read_word(FILE *in) {
    char *string = NULL;
    size_t len = 0;
    int character;

    do {
        if(len % BUFFER == 0)
            string = realloc(string, (len / BUFFER + 1) * BUFFER + 1);

        character = fgetc(in);

        if(character != CARRIAGE_RETURN and character != QUOTE)
            string[len++] = character;

    } while (character != SPACE and character != BREAK_LINE and character != EOF);

    string[len-1] = '\0';
    string = realloc(string, len);
    return string;
}
