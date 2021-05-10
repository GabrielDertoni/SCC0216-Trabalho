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

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#define member_sizeof(ty, member) sizeof(((ty *)NULL)->member)

#define _EXPAND_TYPE_char           TYPE_CHAR
#define _EXPAND_TYPE_i32            TYPE_I32
#define _EXPAND_TYPE_int            TYPE_I32
#define _EXPAND_TYPE_double         TYPE_F64
#define _EXPAND_TYPE_f64            TYPE_F64
#define _EXPAND_TYPE_string         TYPE_STR
#define _EXPAND_TYPE_any            TYPE_ANY
#define _EXPAND_TYPE_void           TYPE_NONE

#define _EXPAND_TYPE(ty) _EXPAND_TYPE_##ty

// Macros que ajudam na legibilidade das funções que espelham um struct.

#define csv_model(strct, n)     csv_new(sizeof(strct), n);
#define csv_get_values(csv, ty) ((ty *)csv_get_raw_values(csv))

#define csv_column(ty, strct, member) \
    csv_column_new( \
        _EXPAND_TYPE(ty), \
        member_sizeof(strct, member) / csv_sizeof_type(_EXPAND_TYPE(ty)), \
        offsetof(strct, member), \
        NULL \
    )

#define csv_column_default(ty, strct, member, val) \
    csv_column_new_with_default( \
        _EXPAND_TYPE(ty), \
        member_sizeof(strct, member) / csv_sizeof_type(_EXPAND_TYPE(ty)), \
        offsetof(strct, member), \
        NULL, \
        (Value)val \
    )

// Todos os tipos primitivos suportados. Todos os tipos que uma célula do CSV
// pode assumir.
typedef enum {
    TYPE_I32,
    TYPE_F64,
    TYPE_CHAR,
    TYPE_STR,
    TYPE_ANY,
    TYPE_NONE,
} PrimitiveType;

typedef union {
    int32_t i32;
    double  f64;
    char    chr;
    char *  str;
    void *  any;
} Value;

typedef enum {
    CSV_ERR_FILE,
    CSV_ERR_PARSE,
    CSV_OK,
} CSVResult;

typedef struct {
    PrimitiveType type;
    size_t size;        // Tamanho do array de `type`. Utilizado apenas para `char[n]`.
    size_t offset;
    char *name;
    bool is_required;
    Value default_val;
} Column; 

typedef struct {
    Column *columns;
    size_t n_columns;
    size_t n_rows;
    size_t capacity;
    size_t elsize;
    void *values;
    char *error_msg;
} CSV;

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
 * @param sep - o separador de campo utilizado pelo arquivo. [ref]
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
 * @param size - o número de tipos `type` que devem estar presentes. Só
 *               utilizado para tipos char.
 * @param name - o nome do campo. Providenciar NULL faz com que o nome da coluna
 *               seja lido do cabeçalho do CSV. [ref]
 */
Column csv_column_new(PrimitiveType type, size_t size, size_t offset, const char *name);

/**
 * Cria uma nova coluna com algum valor padrão. Para criar a coluna, o parâmetro
 * `name` é duplicado.
 *
 * @param type - o tipo primitivo da coluna.
 * @param offset - o offset desde o começo do struct espelhado.
 * @param name - o nome do campo. Providenciar NULL faz com que o nome da coluna
 *               seja lido do cabeçalho do CSV. [ref]
 * @param default_val - o valor padrão da coluna.
 */
Column csv_column_new_with_default(PrimitiveType type, size_t size, size_t offset, const char *name, Value default_val);

/* Outros */

/**
 * Converte um tipo primitivo em uma string que o representa.
 *
 * @param type - o tipo a ser convertido em string.
 * @return a string que representa o tipo. [ref]
 */
char *type_to_string(PrimitiveType type);

/**
 * Imprime o header do CSV gerado a partir das colunas registradas e seus tipos.
 *
 * @param csv - o csv utilizado para a impressão. [ref]
 */
void csv_print_header(CSV *csv);

uint8_t csv_sizeof_type(PrimitiveType type);

#endif
