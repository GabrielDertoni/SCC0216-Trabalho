#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include <utils.h>

/**
 * Mesmo que `alloc_sprintf` mas toma uma `va_list` ao invés de múltiplos
 * argumentos.
 *
 * @param format - o formato da mensagem de acordo com o formato `printf`.
 * @param ap - a lista de argumentos.
 * @return uma string dinamicamente alocada com os conteúdos que seriam
 *         imprimidos pela função `printf`.
 */
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

/**
 * Aloca uma string com os conteúdos que seriam impressos pela função `printf`.
 * A string retornada precisa ser liberada com `free`.
 *
 * @param format - o formatado da mensagem de acordo como formato `printf`.
 * @param ... - parâmetros com os valores que devem ser impressos.
 * @return uma string dinamicamente alocada com os conteúdos que seriam
 *         imprimidos pela função `printf`.
 */
char *alloc_sprintf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);

    char *s = alloc_vsprintf(format, ap);

    va_end(ap);
    return s;
}

/**
 * Lê uma única palavra da entrada padrão separada por espaços e retorna uma
 * string dinamicamente alocada para ela. A string retornada precisa ser
 * liberada com `free`.
 *
 * @param in - o _file pointer_ que será lido.
 * @return uma string dinamicamente alocada que contém uma palavra lida de `in`.
 */
char *read_word(FILE *in) {
    ignore_some(in, " ");

    // Espia o próximo caractere da entrada, se ele for EOF, não há string a ser
    // lida, se for `"`, temos uma string entre aspas e queremos somente a
    // string dentro e não as aspas, isso inclui espaços que estejam dentro das
    // aspas. Se não for uma string entre aspas, retornamos o caractere espiado
    // para a entrada e lemos tudo até o próximo espaço ou quebra de linha.

    int peek = getc(in);
    bool has_open_quote = true;

    if (peek == EOF) {
        return NULL;
    } else if (peek != '"') {
        ungetc(peek, in);
        has_open_quote = false;
    }

    char *string;
    if (has_open_quote) {
        // Lê tudo até o fecha aspas, não possui suporte para _escape sequences_.
        string = read_until(in, "\"");

        // Consome o fecha aspas.
        getc(in);
    } else {
        string = read_until(in, " \r\n");
    }
    return string;
}

/**
 * Lê uma única palavra da entrada padrão separada por espaços e descarta o que
 * for lido e retorna o número de caracteres ignorados.  Essa função também
 * ignora aspas.
 *
 * @param in - o _file pointer_ que será lido.
 * @return o número de caracteres ignorados do arquivo `in`.
 */
int ignore_word(FILE *in) {
    ignore_some(in, " ");

    // Espia o próximo caractere da entrada, se ele for EOF, não há string a ser
    // lida, se for `"`, temos uma string entre aspas e queremos somente a
    // string dentro e não as aspas, isso inclui espaços que estejam dentro das
    // aspas. Se não for uma string entre aspas, retornamos o caractere espiado
    // para a entrada e lemos tudo até o próximo espaço ou quebra de linha.

    int peek = getc(in);
    bool has_open_quote = true;
    int count = 1;

    if (peek == EOF) {
        return count - 1;
    } else if (peek != '"') {
        ungetc(peek, in);
        has_open_quote = false;
        count--;
    }

    if (has_open_quote) {
        // Lê tudo até o fecha aspas, não possui suporte para _escape sequences_.
        count += ignore_until(in, "\"");

        // Consome o fecha aspas.
        getc(in);
        count += 1;
    } else {
        count += ignore_until(in, " \r\n");
    }
    return count;
}

/**
 * Lê até que algum delimitador seja encontrado. Retorna o que foi lido. Essa
 * função não consome o caractere delimitador da entrada. A string é
 * dinamicamente alocada e precisa ser liberada com `free`.
 *
 * @param in - arquivo de onde ler.
 * @param delim - string com todos os delimitadores que são condição de parada
 *                da leitura.
 * @returns a string lida.
 */
char *read_until(FILE *in, const char *delim) {
    char *string = NULL;
    size_t len = 0;
    size_t cap = 0;
    int chr;

    // Lê até que `strchr(delim, chr)` seja `NULL`, ou EOF seja encontrado. Ou
    // seja, somente quando `chr` não for encontrado em `delim`, paramos a
    // leitura, ou se chegarmos no fim da entrada.
    for (chr = getc(in); chr != EOF && !strchr(delim, chr); chr = getc(in)) {
        if (len >= cap) {
            cap = cap == 0 ? 16 : cap * 2;
            string = (char *)realloc(string, (cap + 1) * sizeof(char));
        }
        string[len++] = chr;
    }

    if (chr != EOF) {
        // `chr` é um caractere delimitador, não queremos consumir esse
        // caractere já que ele pode ser usado em outra situação, então
        // retornamos ele à entrada.
        ungetc(chr, in);
    }

    return string;
}

/**
 * Lê até que algum delimitador seja encontrado. Descarta os caracteres lidos.
 * Essa função não consome o caractere delimitador da entrada.
 *
 * @param in - arquivo de onde ler.
 * @param delim - string com todos os delimitadores que são condição de parada
 *                da leitura.
 */
int ignore_until(FILE *in, const char *delim) {
    int count = 0;
    int chr;

    for (chr = getc(in); chr != EOF && !strchr(delim, chr); chr = getc(in)) {
        count++;
    }

    if (chr != EOF) {
        ungetc(chr, in);
    }
    return count;
}

/**
 * Lê apenas alguns caracteres. Descarta os caracteres lidos. Essa função
 * consome apenas caracteres contidos em `chars`.
 *
 * @param in - arquivo de onde ler.
 * @param chars - apenas caracteres contidos em `chars` serão lidos por essa
 *                função.
 */
int ignore_some(FILE *in, const char *chars) {
    int count = 0;
    int chr;

    for (chr = getc(in); chr != EOF && strchr(chars, chr); chr = getc(in)) {
        count++;
    }

    if (chr != EOF) {
        ungetc(chr, in);
    }
    return count;
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

