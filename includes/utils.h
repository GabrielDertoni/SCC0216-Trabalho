/**
 * Módulo com funções utilitárias.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

typedef int32_t(*__compare_function__)(void*, int32_t, int32_t);

/**
 * Aloca uma string com os conteúdos que seriam impressos pela função `printf`.
 * A string retornada precisa ser liberada com `free`.
 *
 * @param format - o formatdo da mensagem de acordo como formato `printf`.
 * @param ... - parâmetros com os valores que devem ser impressos.
 * @return uma string dinâmicamente alocada com os conteúdos que seriam
 *         imprimidos pela função `printf`.
 */
char *alloc_sprintf(const char *format, ...);

/**
 * Mesmo que `alloc_sprintf` mas toma uma `va_list` ao invés de múltiplos
 * argumentos.
 *
 * @param format - o formato da mensagem de acordo com o formato `printf`.
 * @param ap - a lista de argumentos.
 * @return uma string dinamicamente alocada com os conteúdos que seriam
 *         imprimidos pela função `printf`.
 */
char *alloc_vsprintf(const char *format, va_list ap);

/**
 * Lê uma única palavra da entrada padrão separada por espaços e retorna uma
 * string dinamicamente alocada para ela. A string retornada precisa ser
 * liberada com `free`. Essa função também ignora aspas.
 *
 * @param in - o _file pointer_ que será lido.
 * @return uma string dinamicamente alocada que contém uma palavra lida de `in`.
 */
char *read_word(FILE *in);

/**
 * Lê uma única palavra da entrada padrão separada por espaços e descarta o que
 * for lido e retorna o número de caracteres ignorados.  Essa função também
 * ignora aspas.
 *
 * @param in - o _file pointer_ que será lido.
 * @return o número de caracteres ignorados do arquivo `in`.
 */
int ignore_word(FILE *in);


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
char *read_until(FILE *in, const char *delim);

/**
 * Lê até que algum delimitador seja encontrado. Descarta os caracteres lidos.
 * Essa função não consome o caractere delimitador da entrada.
 *
 * @param in - arquivo de onde ler.
 * @param delim - string com todos os delimitadores que são condição de parada
 *                da leitura.
 * @returns o número de caracteres ignorados.
 */
int ignore_until(FILE *in, const char *delim);

/**
 * Lê apenas alguns caracteres. Descarta os caracteres lidos. Essa função
 * consome apenas caracteres contidos em `chars`.
 *
 * @param in - arquivo de onde ler.
 * @param chars - apenas caracteres contidos em `chars` serão lidos por essa
 *                função.
 * @returns o número de caracteres ignorados.
 */
int ignore_some(FILE *in, const char *chars);

/*
* Função de ordenação mergesort, O(log_2(n))
* @param data - endereço de memória a ser ordenado
* @param s_items - tamanho dos itens que serão ordenados (geralmente passado por sizeof(data))
* @param begin - inicio do intervalo de dados a serem ordenados
* @param end - fim do intervalo de dados a serem ordenados (se for utilizado até a última posição de data, 
                envie como len(data)-1 para evitar acesso a posições indevidas)
* @param cmp - função de comparação
*/
void mergesort(void *data, int s_items, int begin, int end, __compare_function__ cmp);

#endif
