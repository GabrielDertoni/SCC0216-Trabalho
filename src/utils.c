#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include<string.h>
#include<utils.h>

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

void merge(void *data, int s_items, int begin, int half, int end, __compare_function__ cmp){
    char *copy_data = malloc((end - begin + 1) * s_items);
    int i = begin, j = half+1, k = 0;
    while(i <= half && j <= end){
        int result = cmp(data, i, j);
        if(result <= 0){
            memcpy(&copy_data[k*s_items], &((char*)data)[i*s_items], s_items);
            i++;
        }
        else{
            memcpy(&copy_data[k*s_items], &((char*)data)[j*s_items], s_items);
            j++;
        }
        k++;
    }

    while(i <= half){
        memcpy(&copy_data[k*s_items], &((char*)data)[i*s_items], s_items);
        i++;
        k++;
    }

    while(j <= end){
        memcpy(&copy_data[k*s_items], &((char*)data)[j*s_items], s_items);
        j++;
        k++;
    }
    for(int i = begin, k = 0; i <= end; i++, k++){
        memcpy(&((char*)data)[i*s_items], &copy_data[k*s_items], s_items);
    }

    free(copy_data);
}

void mergesort(void *data, int s_items, int begin, int end, __compare_function__ cmp){
    if(end <= begin)
        return;
    int half = begin + (int)((end - begin) / 2);
    mergesort(data, s_items, begin, half, cmp);
    mergesort(data, s_items, half+1, end, cmp);
    merge(data, s_items, begin, half, end, cmp);
}