#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <common.h>
#include <parsing.h>
#include <csv.h>
#include <bin.h>

// Imprime um erro do csv.
void print_error(CSV *csv) {
    // fprintf(stderr, "Error: %s.\n", csv_get_error(csv));
    fprintf(stderr, ERROR_FOUND);
}

// Lê um número que caiba em 32 bits.
static CSVResult parse_i32(CSV *csv, const char *input, int32_t *field) {
    if (!strcmp(input, NULL_VAL)) {
        *field = -1;
        return CSV_OK;
    }

    char *endptr;
    long num = strtol(input, &endptr, 10);

    if (endptr == input) {
        csv_error_curr(csv, "expected a number, but found '%s'", input);
        return CSV_ERR_PARSE;
    } else if (endptr[0] != '\0') {
        csv_error_curr(csv, "could not parse entire field '%s'", input);
        return CSV_ERR_PARSE;
    }

    if (num > INT32_MAX) {
        csv_error_curr(csv, "number does not fit in 32 bit integer '%s'", input);
        return CSV_ERR_PARSE;
    }

    *field = num;
    return CSV_OK;
}

// Lê uma string de tamanho fixo com valor padrão. Opcionalmente a string pode
// estar entre aspas duplas.
static CSVResult parse_static_string(
    CSV *csv,
    const char *input,
    char *field,
    size_t size,
    const char *default_val
) {
    if (!strcmp(input, NULL_VAL)) {
        memcpy(field, default_val, size);
        return CSV_OK;
    }

    bool is_quoted = input[0] == '"';
    if (is_quoted) {
        input++;
    }

    size_t len = strlen(input);

    const char *closing;
    // Encontra uma '"' que não tenha '\\' antes.
    do {
        closing = strchr(&input[1], '"');
    } while (closing && closing[-1] == '\\');

    if (!closing) {
        if (is_quoted) {
            csv_error_curr(csv, "expected closing quote");
            return CSV_ERR_PARSE;
        }
        closing = input + len;
    } else {
        if (closing[1] != '\0') {
            csv_error_curr(csv, "expected closing quote to be at the end of field");
            return CSV_ERR_PARSE;
        }
        len = closing - input;
    }

    if (len != size) {
        csv_error_curr(
            csv,
            "expected static string to be of size %zu but was of size %zu",
            size,
            len
        );
        return CSV_ERR_PARSE;
    }

    memcpy(field, input, size);
    return CSV_OK;
}

// Lê uma string de tamanho variável mas que caiba dentro de um campo de tamanho
// fixo.
static CSVResult parse_static_dynamic_string(
    CSV *csv,
    const char *input,
    char *field,
    size_t max_size,
    char *default_val
) {
    if (!strcmp(input, NULL_VAL)) {
        memcpy(field, default_val, strlen(default_val) * sizeof(char));
        return CSV_OK;
    }

    size_t len = strlen(input);
    if (len >= max_size) {
        csv_error_curr(
            csv,
            "expected field to have at most %zu characters, but has %zu",
            max_size,
            len
        );
        return CSV_ERR_PARSE;
    }

    strcpy(field, input);
    return CSV_OK;
}

// Lê uma string de qualquer tamanho.
static CSVResult parse_dynamic_string(CSV *csv, const char *input, char **field) {
    if (!strcmp(input, NULL_VAL)) {
        *field = NULL;
        return CSV_OK;
    }

    bool is_quoted = input[0] == '"';
    if (is_quoted) {
        input++;
    }

    char *closing;
    // Encontra uma '"' que não tenha '\\' antes.
    do {
        closing = strchr(&input[1], '"');
    } while (closing && closing[-1] == '\\');

    if (closing) {
        if (closing[1] != '\0') {
            csv_error_curr(csv, "expected closing quote to be at the end of field");
            return CSV_ERR_PARSE;
        }
        *field = strndup(input, closing - input);
    } else {
        if (is_quoted) {
            csv_error_curr(csv, "expected closing quote");
            return CSV_ERR_PARSE;
        }
        *field = strdup(input);
    }

    return CSV_OK;
}

// Lê o campo "prefixo" de um registro de veículo. Wrapper para `parse_static_string`.
static CSVResult vehicle_parse_prefixo(CSV *csv, const char *input, char (*field)[5]) {
    return parse_static_string(csv, input, (char *)field, sizeof(*field), "\0@@@@");
}

// Lê o campo "data" de um registro de veículo. Wrapper para `parse_static_string`.
static CSVResult vehicle_parse_data(CSV *csv, const char *input, char (*field)[10]) {
    return parse_static_string(csv, input, (char *)field, sizeof(*field), "\0@@@@@@@@@");
}

CSV configure_vehicle_csv() {
    CSV csv = csv_model(Vehicle, 6);

    csv_set_column(&csv, 0, csv_column(Vehicle, prefixo          , csv_static_field(vehicle_parse_prefixo)));
    csv_set_column(&csv, 1, csv_column(Vehicle, data             , csv_static_field(vehicle_parse_data)));
    csv_set_column(&csv, 2, csv_column(Vehicle, quantidadeLugares, csv_static_field(parse_i32)));
    csv_set_column(&csv, 3, csv_column(Vehicle, codLinha         , csv_static_field(parse_i32)));
    csv_set_column(&csv, 4, csv_column(Vehicle, modelo           , csv_dynamic_field(parse_dynamic_string)));
    csv_set_column(&csv, 5, csv_column(Vehicle, categoria        , csv_dynamic_field(parse_dynamic_string)));

    return csv;
}

static CSVResult bus_line_parse_aceitaCartao(CSV *csv, const char *input, char (*field)[1]) {
    return parse_static_string(csv, input, (char *)field, sizeof(*field), "\0");
}

static CSVResult bus_line_parse_codLinha(CSV *csv, const char *input, char (*field)[32]) {
    return parse_static_dynamic_string(csv, input, (char *)field, sizeof(*field), "-1\0");
}

CSV configure_bus_line_csv() {
    CSV csv = csv_model(BusLine, 4);

    csv_set_column(&csv, 0, csv_column(BusLine, codLinha    , csv_static_field(bus_line_parse_codLinha)));
    csv_set_column(&csv, 1, csv_column(BusLine, aceitaCartao, csv_static_field(bus_line_parse_aceitaCartao)));
    csv_set_column(&csv, 2, csv_column(BusLine, nomeLinha   , csv_dynamic_field(parse_dynamic_string)));
    csv_set_column(&csv, 3, csv_column(BusLine, corLinha    , csv_dynamic_field(parse_dynamic_string)));

    return csv;
}

CSVResult vehicle_row_iterator(CSV *csv, const Vehicle *vehicle, IterArgs *args) {
    if (!write_vehicle(vehicle, args->fp)) {
        csv_error(csv, "failed to write vehicle");
        return CSV_ERR_OTHER;
    } else {
        if (vehicle->prefixo[0] == REMOVED_MARKER) {
            args->removed_reg_count++;
        } else {
            args->reg_count++;
        }

        return CSV_OK;
    }
}

CSVResult bus_line_row_iterator(CSV *csv, const BusLine *bus_line, IterArgs *args) {
    if (!write_bus_line(bus_line, args->fp)) {
        csv_error(csv, "failed to write bus line");
        return CSV_ERR_OTHER;
    } else {
        if (bus_line->codLinha[0] == REMOVED_MARKER) {
            args->removed_reg_count++;
        } else {
            args->reg_count++;
        }

        return CSV_OK;
    }
}

// Uma nota sobre `goto`
//
// Nesse caso, `goto` é utilizado para reduzir código repetido. Sempre que há
// algum erro, queremos liberar o csv no qual estamos operando e fechar o
// arquivo binário. Isso também é feito ao final da função.
//
// Usando `goto`, fica explícito que quado ocorre algum erro, queremos pular
// todo o resto e ir direto para a parte que liberamos tudo.
//
// Alternativamente teríamos que replicar a operação de liberar tudo e retornar
// a cada verificação de erro o que aumenta a chance de errar no código.
// 
// Nos macros `do while` só é utilizado para que possamos escrever um ';' depois
// da utilização do macro.

// Verifica se uma determinada expressão é um erro. Se for, imprime o erro e
// pula para o label `teardown`.
#define CSV_ASSERT(expr)          \
    do {                          \
        if (CSV_IS_ERROR(expr)) { \
            print_error(csv);     \
            goto teardown;        \
        }                         \
    } while (0)

// Verifica se um determinado valor é diferente de 0. Se for, vai diretamente
// para o label `teardown`.
#define ASSERT_OR(expr, ...)              \
    do {                                  \
        if ((expr) != 0) {                \
            fprintf(stderr, __VA_ARGS__); \
            goto teardown;                \
        }                                 \
    } while (0)

// Converte um csv para um arquivo binário de registros. A leitura do csv é
// controlada por `csv` e a escrita no binário é controlada por `iter`. O
// argumento `iter` tem que ser `vehicle_from_stdin_append_to_bin` ou
// `bus_line_row_iterator`, por conta dessa restrição essa não é uma função
// completamente genérica. De forma mais geral, `iter` pode ser qualquer função
// se encaixe nas restrições de `InterFunc` e receba como terceiro argumento um
// ponteiro do tipo `IterArgs`.
//
// Essa função assume que o csv possui um header ainda por ser lido. Além disso,
// os nomes das colunas são escritos em ordem e sem delimitador logo após o meta
// header no arquivo binário. Ou seja, o leitor do arquivo binário gerado tem
// que saber de antemão o tamanho do nome de cada coluna.
static bool csv_to_bin(
    CSV *csv,
    const char *bin_fname,
    IterFunc *iter,
    const char *sep
) {
    CSVResult res = CSV_OK;

    FILE *fp = fopen(bin_fname, "w");

    if (!fp) {
        fprintf(stderr, "Error: Could not open file.\n");
        return false;
    }

    CSV_ASSERT(res = csv_parse_header(csv, sep));

    // Primeira escrita, escreve o byte de status para garantir que outros
    // processos tentando ler o arquivo não leiam algo incompleto.
    ASSERT_OR(
        res = !update_header_status('0', fp),
        // "Error: could not write status to file %s.",
        // bin_fname
        ERROR_FOUND
    );

    // Pula o meta header. Como nem todas as informações são conhecidas nesse
    // momento, primeiro processamos as linhas e depois voltamos para escrever o
    // header.
    fseek(fp, META_SIZE, SEEK_SET);

    // Escreve os nomes das colunas em sequência sem \0 e sem verificar tamanho.
    // Portanto erros nesses nomes não serão detectados.
    for (int i = 0; i < csv->n_columns; i++) {
        const char *name = csv_get_col_name(csv, i);
        size_t len = strlen(name);

        ASSERT_OR(
            res = !fwrite(name, len * sizeof(char), 1, fp),
            // "Error: could not write column name to file %s.",
            // bin_fname
            ERROR_FOUND
        );
    }

    // Configura os argumentos do iterador. Esses valores serão modificados para
    // conter alguns dados que só podem ser contados lendo todos os registros.
    IterArgs args = {
        .fp                = fp,
        .reg_count         = 0,
        .removed_reg_count = 0,
    };

    // Itera por todas as linhas do csv e escreve os registros no binário.
    CSV_ASSERT(res = csv_iterate_rows(csv, sep, iter, &args));

    DBMeta meta = {
        .status          = '1',
        .byteProxReg     = ftell(fp),
        .nroRegistros    = args.reg_count,
        .nroRegRemovidos = args.removed_reg_count,
    };

    // Volta e atualiza o meta header.
    ASSERT_OR(
        res = !update_header_meta(meta, fp),
        // "Error: could not write the meta header to file %s.",
        // bin_fname
        ERROR_FOUND
    );

teardown:
    // Libera os valores abertos/alocados.
    fclose(fp);

    // Nesse momento, `res = 0` somente se não houve erro. Mas nesse caso
    // queremos retornar `true`.
    return !res;
}

bool vehicle_csv_to_bin(const char *csv_fname, const char *bin_fname) {
    CSV csv = configure_vehicle_csv();
    bool res = !csv_open(&csv, csv_fname);

    if (res)
        res = csv_to_bin(&csv, bin_fname, (IterFunc *)vehicle_row_iterator, ",");

    if (!res)
        print_error(&csv);

    csv_drop(csv);
    return res;
}

bool bus_line_csv_to_bin(const char *csv_fname, const char *bin_fname) {
    CSV csv = configure_bus_line_csv();
    bool res = !csv_open(&csv, csv_fname);

    if (res)
        res = csv_to_bin(&csv, bin_fname, (IterFunc *)bus_line_row_iterator, ",");

    if (!res)
        print_error(&csv);

    csv_drop(csv);
    return res;
}

// Lê as linhas de um csv com campos separados por `sep` e escreve os registros
// lidos no arquivo binário de nome `bin_fname`. A função `iter` tem que ser
// `vehicle_row_iterator` ou `bus_line_row_iterator`.
//
// Considera que não há header no csv.
static bool csv_append_to_bin(const char *bin_fname, CSV *csv, IterFunc *iter, const char *sep) {
    FILE *fp = fopen(bin_fname, "r+b");

    if (!fp) {
        // fprintf(stderr, "Error: Could not open file.\n");
        fprintf(stderr, ERROR_FOUND);
        return false;
    }

    CSVResult res = CSV_OK;
    DBMeta meta;

    ASSERT_OR(
        res = !read_meta(fp, &meta),
        // "Error: could not read meta header from file '%s'.",
        // bin_fname
        ERROR_FOUND
    );

    ASSERT_OR(
        res = !update_header_status('0', fp),
        // "Error: could not write status to file '%s'.",
        // bin_fname
        ERROR_FOUND
    );

    IterArgs args = {
        .fp                = fp,
        .reg_count         = 0,
        .removed_reg_count = 0,
    };

    // Vai para o fim do arquivo para adicionar novos registros.
    fseek(fp, 0L, SEEK_END);

    CSV_ASSERT(res = csv_iterate_rows(csv, sep, (IterFunc *)iter, &args));

    meta.status = '1';
    meta.byteProxReg = ftell(fp);
    meta.nroRegRemovidos += args.removed_reg_count;
    meta.nroRegistros += args.reg_count;

    ASSERT_OR(
        res = !update_header_meta(meta, fp),
        // "Error: could not write meta header to file '%s'.",
        // bin_fname
        ERROR_FOUND
    );

teardown:
    fclose(fp);

    return !res;
}

bool vehicle_append_to_bin_from_stdin(const char *bin_fname) {
    CSV csv = configure_vehicle_csv();
    csv_use_fp(&csv, stdin);

    bool res = csv_append_to_bin(bin_fname, &csv, (IterFunc *)vehicle_row_iterator, " ");

    csv_drop(csv);
    return res;
}

bool bus_line_append_to_bin_from_stdin(const char *bin_fname) {
    CSV csv = configure_bus_line_csv();
    csv_use_fp(&csv, stdin);

    bool res = csv_append_to_bin(bin_fname, &csv, (IterFunc *)bus_line_row_iterator, " ");

    csv_drop(csv);
    return res;
}
