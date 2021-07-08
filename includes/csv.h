/**
 * Módulo de leitura de CSV.
 *
 * Esse módulo é genérico para qualquer CSV. Ele pode ser utilizado para carregar
 * CSVs de maneira rápida e eficiente e consegue armazenar cada linha do CSV num
 * formato que espelha algum struct.
 * Ou seja, na memória os dados do CSV serão carregados com o mesmo alinhamento
 * e offsets configurados através da função `csv_set_column`. Isso permite uma
 * conversão direta (sem custo) para o struct espelhado.
 *
 * Note que:
 *  - Antes de carregar o CSV, todas as colunas devem ter sido registradas.
 *  - O CSV deve ter uma primeira linha de header que define o nome de cada coluna.
 */

#ifndef __CSV_H__
#define __CSV_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

// Calcula o tamanho do membro de um struct.
#define _member_sizeof(ty, member) sizeof(((ty *)NULL)->member)

// Macros que ajudam na legibilidade das funções que espelham um struct.

// Cria um novo `CSV` de acordo com o struct `strct` e o seu número de campos.
#define csv_model(strct, n)     csv_new(sizeof(strct), n);

// Retorna os valores armazenados no buffer do csv com o tipo fornecido. Esses
// valores ainda pertencem ao `csv` e **não podem ser liberados**.
#define csv_get_values(csv, ty) ((ty *)csv_get_raw_values(csv))

/**
 * `csv_dynamic_field` e `csv_static_field` devem ser usadas como terceiro
 * argumento do macro `csv_column`.
 * Elas acoplam uma função de leitura com uma função de liberação. Ou seja, se
 * uma determinada função de leitura faz alguma alocação dinâmica, ela deverá
 * ser usada juntamente com `csv_dynamic_field`, caso contrário ela deverá ser
 * usada juntamente com `csv_static_field`.
 */
#define csv_dynamic_field(parse) (CSVParseFunc *)parse, (CSVDropFunc *)free
#define csv_static_field(parse)  (CSVParseFunc *)parse, (CSVDropFunc *)NULL

// Expande no macro `csv_column`.
#define _csv_column(strct, member, parse, drop) \
    csv_column_new(                             \
        _member_sizeof(strct, member),          \
        offsetof(strct, member),                \
        NULL,                                   \
        parse,                                  \
        drop                                    \
    )

// Cria uma nova coluna do csv.
#define csv_column(strct, member, args...) \
    _csv_column(strct, member, args)

#define CSV_IS_ERROR(res) (res != 0)

typedef enum {
    CSV_OK        = 0,
    CSV_ERR_OTHER = 1,
    CSV_ERR_FILE,
    CSV_ERR_PARSE,
    CSV_ERR_EOF,
} CSVResult;

typedef struct CSVColumn CSVColumn;

typedef struct {
    CSVColumn *columns;
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

typedef CSVResult (CSVParseFunc)(CSV *, const char *, void *);
typedef void (CSVDropFunc)(void *);
typedef CSVResult (CSVIterFunc)(CSV *, const void *, void *arg);

struct CSVColumn {
  size_t size;
  size_t offset;
  char *name;
  CSVParseFunc *parse;
  CSVDropFunc *drop;
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
 * Libera a memória alocada pelo csv. Se há algum arquivo csv aberto, fecha o
 * arquivo.
 *
 * @param csv - o csv a ser liberado.
 */
void csv_drop(CSV csv);

/**
 * Configura o erro do csv de acordo com uma mensagem. Essa função possui um
 * formato igual a da família `printf`. Se já havia alguma mensagem
 * anteriormente, ela é liberada e a nova mensagem é escrita em seu lugar.
 *
 * @param csv - o csv em que ocorreu erro.
 * @param format - o formato da mensagem de erro, segue o mesmo padrão que a
 *                 família `printf`.
 * @param ... - outros argumentos que com os dados especificados em `format`.
 */
void csv_error(CSV *csv, const char *format, ...);

/**
 * Configura o erro do csv de acordo com uma mensagem e adiciona informações
 * sobre o ponto da leitura onde `csv` está. Essas informações são: nome do
 * arquivo, linha e número do campo. Essa função possui um formato igual a da
 * família `printf`. Se já havia alguma mensagem anteriormente, ela é liberada e
 * a nova mensagem é escrita em seu lugar.
 *
 * @param csv - o csv em que ocorreu erro.
 * @param format - o formato da mensagem de erro, segue o mesmo padrão que a
 *                 família `printf`.
 * @param ... - outros argumentos que com os dados especificados em `format`.
 */
void csv_error_curr(CSV *csv, const char *format, ...);

/**
 * Usa um ponteiro de arquivo já aberto. Assume que o ponteiro de arquivo tem
 * permissão de leitura. Em caso de erro, seta a mensagem de erro do `csv` com
 * um erro apropriado.
 *
 * @param csv - o tipo que usará o ponteiro de arquivo.
 * @param fp - o ponteiro de arquivo para uso.
 * @return `CSV_OK` caso haja sucesso e `CSV_ERR_FILE` em caso de erro.
 */
CSVResult csv_use_fp(CSV *csv, FILE *fp);

/**
 * Abre um arquivo .csv e registra num tipo `CSV`. O arquivo será aberto em
 * forma de leitura. Em caso de erro, seta a mensagem de erro do `csv` com um
 * erro apropriado.
 *
 * @param csv - o tipo para onde o arquivo será aberto.
 * @param fname - o nome do arquivo a ser aberto.
 * @return retorna `CSV_OK` caso haja sucesso e `CSV_ERR_FILE` em caso de erro.
 */
CSVResult csv_open(CSV *csv, const char *fname);

/**
 * Fecha o arquivo. Em caso de erro, seta a mensagem de erro do `csv` com um
 * erro apropriado.
 *
 * @param csv - o tipo csv que possui o arquivo aberto. [ref mut]
 * @return retorna `CSV_OK` caso haja sucesso e `CSV_ERR_FILE` em caso de erro.
 */
CSVResult csv_close(CSV *csv);

/**
 * Executa uma determinada função para cada registro lido do .csv. Considera que
 * o header já foi lido ou que não há header no arquivo.
 *
 * @param csv - o csv a ser iterado.
 * @param sep - o separador de campos usado no csv.
 * @param iter - a função a ser executada para cada registro lido.
 * @param arg - um argumento adicional que será providenciado à função `iter`.
 * @return retorna `CSV_OK` apenas se todas as linhas forem lidas corretamente e
 *         o retorno de todas as execuções de `iter` tenham resultado em sucesso.
 *         Em caso de erro, retorna um erro apropriado.
 */
CSVResult csv_iterate_rows(CSV *csv, const char *sep, CSVIterFunc *iter, void *arg);

/**
 * Lê uma linha do .csv e escreve o resultado em `strct`. Assume que haja um
 * arquivo aberto no `csv`.
 *
 * @param csv - o csv a ser usado para leirura.
 * @param strct - um pointeiro à estrutura espelhada por `csv`.
 * @param sep - o separador de campos do arquivo.
 * @return retorna `CSV_OK` se a leitura for bem sucedida e um valor de erro
 *         caso contrário.
 */
CSVResult csv_parse_next_row(CSV *csv, void *strct, const char *sep);

/**
 * Lê o header do csv. Assume que haja um arquivo aberto no `csv`.
 *
 * @param csv - o csv a ser usado para a leitura.
 * @param sep - o separador de campos do arquivo.
 * @return retorna `CSV_OK` em caso de sucesso e um valor de erro em caso
 *         contrário.
 */
CSVResult csv_parse_header(CSV *csv, const char *sep);

/**
 * Registra uma nova coluna para o csv.
 *
 * @param csv - o csv no qual se deseja registrar a nova coluna. [mut ref]
 * @param idx - o índice da coluna a ser configurada.
 * @param column - as configurações da coluna gerada através do `csv_column_new`.
 */
void csv_set_column(CSV *csv, size_t idx, CSVColumn column);

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
 * Retorna a mensagem de erro do `csv`.
 *
 * @param csv - o csv erro.
 * @return uma string contendo a mensagem de erro. Pode ser `NULL` caso não haja
 *         erro.
 */
const char *csv_get_error(const CSV *csv);

/**
 * Retorna o nome de uma determinada coluna.
 *
 * @param csv - o csv que será acessado.
 * @param idx - o índice da coluna a ser acessada.
 * @return o nome da coluna. Pode ser `NULL`.
 */
const char *csv_get_col_name(const CSV *csv, int idx);

/**
 * Verifica se o `CSV` possui algum arquivo aberto.
 *
 * @param csv - o csv a verificar.
 * @return `true` se há um arquivo aberto. `false` caso contrário.
 */
bool csv_is_open(const CSV *csv);

/**
 * Retorna o nome do arquivo csv aberto atualmente.
 *
 * @param csv - o csv que será acessado.
 * @return o nome do arquivo aberto. Pode ser `NULL`.
 */
const char *csv_get_fname(const CSV *csv);

/**
 * Retorna o número do campo na leitura atual do csv.
 *
 * @param csv - o csv que será acessado.
 * @return o número do campo atual. Esse valor começa em 0.
 */
size_t csv_get_curr_field(const CSV *csv);

/**
 * Retorna o número da linha do arquivo na leitura atual do csv.
 *
 * @param csv - o csv que será acessado.
 * @return o número da linha atual. Esse valor começa em 0. Se não há arquivo
 *         aberto, esse valor é indeterminado.
 */
size_t csv_get_curr_line(const CSV *csv);

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
 * @param size - o tamanho do campo.
 * @param offset - o offset desde o começo do struct espelhado.
 * @param name - o nome do campo. Providenciar NULL faz com que o nome da coluna
 *               seja lido do cabeçalho do CSV. [ref]
 * @param parse - função que lê o campo do csv.
 * @param drop - função que libera o tipo lido por `parse`.
 * @return uma nova coluna.
 */
CSVColumn csv_column_new(size_t size, size_t offset, const char *name, CSVParseFunc *parse, CSVDropFunc *drop);

/* Outros */

/**
 * Imprime o header do CSV gerado a partir das colunas registradas e seus tipos.
 *
 * @param csv - o csv utilizado para a impressão. [ref]
 */
void csv_print_header(CSV *csv);

#endif

