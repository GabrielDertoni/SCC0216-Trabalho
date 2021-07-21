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

/*
* Função auxiliar ao mergesort, realiza a ordenação dos itens
* @param data - endereço de memória a ser ordenado
* @param s_items - tamanho dos itens que serão ordenados (geralmente passado por sizeof(data))
* @param begin - inicio do intervalo de dados a serem ordenados
* @param half - meio do intervalo de dados a serem ordenados
* @param end - fim do intervalo de dados a serem ordenados
* @param cmp - função de comparação
*/
void merge(void *data, int s_items, int begin, int half, int end, __compare_function__ cmp){
    char *copy_data = malloc((end - begin + 1) * s_items);
    int i = begin, j = half+1, k = 0;
    // Procura os menores itens entre a primeira e a segunda metade do intervalo
    while(i <= half && j <= end){
        int result = cmp(data, i, j);
        if(result <= 0)
            memcpy(&copy_data[(k++)*s_items], &((char*)data)[(i++)*s_items], s_items);
        else
            memcpy(&copy_data[(k++)*s_items], &((char*)data)[(j++)*s_items], s_items);
    }

    // Copia os valores restantes da primeira metade, se tiver algum
    while(i <= half)
        memcpy(&copy_data[(k++)*s_items], &((char*)data)[(i++)*s_items], s_items);

    // Copia os valores restantes da segunda metade, se tiver algum
    while(j <= end)
        memcpy(&copy_data[(k++)*s_items], &((char*)data)[(j++)*s_items], s_items);

    // Copia o intervalo ordenado de volta para o espaço de memória original
    for(int i = begin, k = 0; i <= end; i++, k++)
        memcpy(&((char*)data)[i*s_items], &copy_data[k*s_items], s_items);

    free(copy_data);
}

/*
* Função de ordenação mergesort, O(log_2(n))
* @param data - endereço de memória a ser ordenado
* @param s_items - tamanho dos itens que serão ordenados (geralmente passado por sizeof(data))
* @param begin - inicio do intervalo de dados a serem ordenados
* @param end - fim do intervalo de dados a serem ordenados (se for utilizado até a última posição de data, 
                envie como len(data)-1 para evitar acesso a posicoes indevidas)
* @param cmp - função de comparação
*/
void mergesort(void *data, int s_items, int begin, int end, __compare_function__ cmp){
    if(end <= begin)
        return;
    // Divide o intervalo no meio
    int half = begin + (int)((end - begin) / 2);
    // Divide em intervalos menores recursivamente
    mergesort(data, s_items, begin, half, cmp);
    mergesort(data, s_items, half+1, end, cmp);
    // Ordena os intervalos
    merge(data, s_items, begin, half, end, cmp);
}