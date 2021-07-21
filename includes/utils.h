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
 * Mesmo que `alloc_sprintf` mas toma uma `va_list` ao invés de multiplos
 * argumentos.
 *
 * @param format - o formato da mensagem de acordo com o formato `printf`.
 * @param ap - a lista de argumentos.
 * @return uma string dinâmicamente alocada com os conteúdos que seriam
 *         imprimidos pela função `printf`.
 */
char *alloc_vsprintf(const char *format, va_list ap);

/**
 * Lê uma única palavra da entrada padrão separada por espaços e retorna uma
 * string dinâmicamente alocada para ela. A string retornada precisa ser
 * liberada com `free`.
 *
 * @param in - o _file pointer_ que será lido.
 * @return uma string dinâmicamente alocada que contém uma palavra lida de `in`.
 */
char *read_word(FILE *in);

/*
* Função de ordenação mergesort, O(log_2(n))
* @param data - endereço de memória a ser ordenado
* @param s_items - tamanho dos itens que serão ordenados (geralmente passado por sizeof(data))
* @param begin - inicio do intervalo de dados a serem ordenados
* @param end - fim do intervalo de dados a serem ordenados (se for utilizado até a última posição de data, 
                envie como len(data)-1 para evitar acesso a posicoes indevidas)
* @param cmp - função de comparação
*/
void mergesort(void *data, int s_items, int begin, int end, __compare_function__ cmp);

#endif
