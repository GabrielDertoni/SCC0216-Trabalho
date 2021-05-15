/**
 * Módulo CSV.
 *
 * Esse módulo é genérico para qualquer CSV. Ele pode ser utilizado para carregar
 * CSVs de maneira rápida e eficiente e consegue armazenar cada linha do CSV num
 * formato que espelha algum struct.
 * Ou seja, na memória os dados do CSV serão carregados com o mesmo alinhamento
 * configurado através da função `csv_set_column`. Isso permite uma conversão
 * direta para o struct espelhado.
 *
 * Note que:
 *  - Antes de carregar o CSV, todas as colunas devem ter sido registradas.
 *  - O CSV deve ter uma primeira linha de header que define o nome de cada coluna.
 *  - Cada coluna deve ter exatamente um tipo.
 *  - Todas as linhas devem possuir valores para todas as colunas (campos de
 *    string podem ficar vazios).
 */

#ifndef __CSV_H__
#define __CSV_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#define member_sizeof(ty, member) sizeof(((ty *)NULL)->member)

// Macros que ajudam na legibilidade das funções que espelham um struct.

#define csv_model(strct, n)     csv_new(sizeof(strct), n);
#define csv_get_values(csv, ty) ((ty *)csv_get_raw_values(csv))

#define csv_dynamic_field(parse) (ParseFunc *)parse, (DropFunc *)free
#define csv_static_field(parse)  (ParseFunc *)parse, (DropFunc *)NULL

#define _csv_column(strct, member, parse, drop) \
    csv_column_new(                             \
        member_sizeof(strct, member),           \
        offsetof(strct, member),                \
        NULL,                                   \
        parse,                                  \
        drop                                    \
    )

#define csv_column(strct, member, args...) \
    _csv_column(strct, member, args)

#define CSV_IS_ERROR(res) (res & 0b1000)

typedef enum {
    CSV_OK        = 0b0000,
    CSV_ERR_FILE  = 0b1000,
    CSV_ERR_PARSE = 0b1001,
    CSV_ERR_EOF   = 0b1010,
    CSV_ERR_OTHER = 0b1011,
} CSVResult;

typedef struct Column Column;

typedef struct {
    Column *columns;
    size_t n_columns;
    size_t curr_line;
    size_t curr_field;
    size_t n_rows;
    size_t capacity;
    size_t elsize;
    void *values;
    char *fname;
    FILE *fp;
    char *error_msg;
} CSV;

typedef CSVResult (ParseFunc)(CSV *, const char *, void *);
typedef void (DropFunc)(void *);
typedef CSVResult (IterFunc)(CSV *, const void *, void *arg);

struct Column {
    size_t size;
    size_t offset;
    char *name;
    ParseFunc *parse;
    DropFunc *drop;
}; 

/**
 * Cria o TAD CSV. Esse tipo precisa ser liberado através da função
 * `csv_drop`.
 *
 * @param elsize - o tamanho do struct a ser espelhado.
 * @param n_columns - número de colunas do CSV, ou número de campos do struct
 *                    espelhado.
 * @return o TAD csv sem nenhuma coluna nem nenhuma linha lida. [ownership]
 */
CSV csv_new(size_t elsize, size_t n_columns);

/**
 * Libera a memória alocada pelo csv.
 *
 * @param csv - o csv a ser liberado.
 */
void csv_drop(CSV csv);

void csv_error(CSV *csv, const char *format, ...);
void csv_error_curr(CSV *csv, const char *format, ...);

CSVResult csv_open(CSV *csv, const char *fname);
CSVResult csv_close(CSV *csv);

CSVResult csv_iterate_rows(CSV *csv, const char *sep, IterFunc *iter, void *arg);
CSVResult csv_parse_next_row(CSV *csv, void *strct, const char *sep);
CSVResult csv_parse_header(CSV *csv, const char *sep);

bool csv_is_open(CSV *csv);
char *csv_get_fname(CSV *csv);
size_t csv_get_curr_field(CSV *csv);
size_t csv_get_curr_line(CSV *csv);

/**
 * Registra uma nova coluna para o csv.
 *
 * @param csv - o csv no qual se deseja registrar a nova coluna. [mut ref]
 * @param idx - o índice da coluna a ser configurada.
 * @param column - as configurações da coluna gerada através do `csv_column_new`.
 */
void csv_set_column(CSV *csv, size_t idx, Column column);

/**
 * Carrega um arquivo csv a partir de seu nome.
 *
 * @param csv - o csv sobre o qual carregar os dados. [mut ref]
 * @param fname - o nome do arquivo. [ref]
 * @param sep - cada char em `sep` é considerado um separador. [ref]
 * @return um tipo que diz se a leitura foi bem ou mal sucedida. Caso o
 *         resultado seja CSV_ERR_PARSE, `csv->error_msg` terá uma mensagem de
 *         erro (dinamicamente alocada).
 */
CSVResult csv_parse_file(CSV *csv, const char *fname, const char *sep);

/* Acesso a informações contidas em CSV. */

/**
 * Retorna os dados carregados. Repare que esses dados retornados ainda
 * pertencem ao tipo CSV e não devem ser liberados ou modificados.
 *
 * @param csv - o csv do qual pegar os dados. [ref]
 * @return vetor de valores armazenados empacotados de acordo com as colunas
 *         registradas. Pode ser convertido para um vetor do struct espelhado. [ref]
 */
void *csv_get_raw_values(const CSV *csv);

/**
 * Retorna o número de linhas lidas.
 *
 * @param csv - o csv para o acesso do número de linhas. [ref]
 * @return o número de linhas lidas.
 */
size_t csv_row_count(const CSV *csv);

/**
 * Retorna o índice de uma coluna com determinado nome.
 *
 * @param csv - o csv no qual o índice será baseado. [ref]
 * @param field_name - o nome do campo a ser procurado. [ref]
 * @return o índice desejado ou -1 caso field_name não seja uma das colunas.
 */
int csv_get_field_index(const CSV *csv, const char *field_name);

/* Coluna */

/**
 * Cria uma nova coluna. Para criar a coluna, o parâmetro `name` é duplicado.
 *
 * @param type - o tipo primitivo da coluna.
 * @param offset - o offset desde o começo do struct espelhado.
 * @param name - o nome do campo. Providenciar NULL faz com que o nome da coluna
 *               seja lido do cabeçalho do CSV. [ref]
 */
Column csv_column_new(size_t size, size_t offset, const char *name, ParseFunc *parse, DropFunc *drop);

/* Outros */

/**
 * Imprime o header do CSV gerado a partir das colunas registradas e seus tipos.
 *
 * @param csv - o csv utilizado para a impressão. [ref]
 */
void csv_print_header(CSV *csv);

#endif

