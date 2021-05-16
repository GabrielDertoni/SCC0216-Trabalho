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
    fprintf(stderr, "Error: %s.\n", csv_get_error(csv));
}

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

static CSVResult parse_char(CSV *csv, const char *input, char *field, char default_val) {
    if (!strcmp(input, NULL_VAL)) {
        *field = default_val;
        return CSV_OK;
    }

    if (input[0] == '\0') {
        csv_error_curr(csv, "unexpected end of field");
        return CSV_ERR_PARSE;
    }

    if (input[1] != '\0') {
        csv_error_curr(csv, "expected field to be a single char");
        return CSV_ERR_PARSE;
    }

    *field = input[0];
    return CSV_OK;
}

static CSVResult parse_static_string(
    CSV *csv,
    const char *input,
    char *field,
    size_t size,
    char *default_val
) {
    if (!strcmp(input, NULL_VAL)) {
        memcpy(field, default_val, size);
        return CSV_OK;
    }

    for (size_t i = 0; i < size; i++) {
        if (!input[i]) {
            csv_error_curr(
                csv,
                "expected static string to be of size %zu but was of size %zu",
                size,
                i
            );
            return CSV_ERR_PARSE;
        }
    }

    if (input[size] != '\0') {
        csv_error_curr(
            csv,
            "expected static string to be of size %zu, but found larger string '%s'",
            size,
            input
        );
        return CSV_ERR_PARSE;
    }

    memcpy(field, input, size);
    return CSV_OK;
}

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

static CSVResult parse_dynamic_string(CSV *csv, const char *input, char **field) {
    if (!strcmp(input, NULL_VAL)) {
        *field = NULL;
    } else {
        *field = strdup(input);
    }

    return CSV_OK;
}

static CSVResult vehicle_parse_prefixo(CSV *csv, const char *input, char (*field)[5]) {
    return parse_static_string(csv, input, (char *)field, sizeof(*field), "\0@@@@");
}

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

static CSVResult bus_line_parse_aceitaCartao(CSV *csv, const char *input, char *field) {
    return parse_char(csv, input, field, 'N');
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
// O `do while` só é utilizado para que possamos escrever um ';' depois da
// utilização do macro.

// Verifica se uma determinada expressão é um erro. Se for, imprime o erro e
// pula para o label `teardown`.
#define CSV_ASSERT(expr)          \
    do {                          \
        if (CSV_IS_ERROR(expr)) { \
            print_error(&csv);    \
            goto teardown;        \
        }                         \
    } while (0)

// Verifica se um determinado valor é diferente de 0. Se for, vai diretamente
// para o label `teardown`.
#define ASSERT_OR(expr, msg, ...)              \
    do {                                       \
        if ((expr) != 0) {                     \
            fprintf(stderr, msg, __VA_ARGS__); \
            goto teardown;                     \
        }                                      \
    } while (0)

bool vehicle_csv_to_bin(const char *csv_fname, const char *bin_fname) {
    CSV csv = configure_vehicle_csv();

    CSVResult res = CSV_OK;

    FILE *fp = fopen(bin_fname, "w");

    if (!fp) {
        fprintf(stderr, "Error: Could not open file.\n");
        return false;
    }

    CSV_ASSERT(res = csv_open(&csv, csv_fname));
    CSV_ASSERT(res = csv_parse_header(&csv, ","));

    // Primeira escrita, escreve o byte de status para garantir que outros
    // processos tentando ler o arquivo não leiam algo incompleto.
    ASSERT_OR(
        res = !update_header_status('0', fp),
        "Error: could not write status to file %s.\n",
        bin_fname
    );

    // Pula o header. Como nem todas as informações são conhecidas nesse
    // momento, primeiro processamos as linhas e depois voltamos para escrever o
    // header.
    fseek(fp, VEHICLE_HEADER_SIZE, SEEK_SET);

    // Configura os argumentos do iterador. Esses valores serão modificados para
    // conter alguns dados que só podem ser contados lendo todos os registros.
    IterArgs args = {
        .fp                = fp,
        .reg_count         = 0,
        .removed_reg_count = 0,
    };

    // Itera por todas as linhas do csv e escreve os registros no binário.
    CSV_ASSERT(res = csv_iterate_rows(&csv, ",", (const IterFunc *)vehicle_row_iterator, &args));

    // Voltamos ao começo do arquivo para escrever as últimas coisas antes de
    // finalizar o processamento.
    fseek(fp, 0L, SEEK_SET);

    DBMeta meta = {
        .status          = '1',
        .byteProxReg     = ftell(fp),
        .nroRegistros    = args.reg_count,
        .nroRegRemovidos = args.removed_reg_count,
    };

    DBVehicleHeader header = { .meta = meta };

    memcpy(&header.descrevePrefixo  , csv_get_col_name(&csv, 0), sizeof(header.descrevePrefixo));
    memcpy(&header.descreveData     , csv_get_col_name(&csv, 1), sizeof(header.descreveData));
    memcpy(&header.descreveLugares  , csv_get_col_name(&csv, 2), sizeof(header.descreveLugares));
    memcpy(&header.descreveLinhas   , csv_get_col_name(&csv, 3), sizeof(header.descreveLinhas));
    memcpy(&header.descreveModelo   , csv_get_col_name(&csv, 4), sizeof(header.descreveModelo));
    memcpy(&header.descreveCategoria, csv_get_col_name(&csv, 5), sizeof(header.descreveCategoria));

    ASSERT_OR(
        res = !write_vehicles_header(header, fp),
        "Error: could not write vehicle header to %s.\n",
        bin_fname
    );

teardown:
    // Libera os valores abertos/alocados.
    fclose(fp);
    csv_drop(csv);

    // Nesse momento, `res = 0` somente se não houve erro. Mas nesse caso
    // queremos retornar `true`.
    return !res;
}

bool bus_line_csv_to_bin(const char *csv_fname, const char *bin_fname) {
    CSV csv = configure_bus_line_csv();
    CSVResult res = CSV_OK;

    FILE *fp = fopen(bin_fname, "w");

    if (!fp) {
        fprintf(stderr, "Error: Could not open file.\n");
        return false;
    }

    CSV_ASSERT(res = csv_open(&csv, csv_fname));
    CSV_ASSERT(res = csv_parse_header(&csv, ","));

    // Primeira escrita, escreve o byte de status para garantir que outros
    // processos tentando ler o arquivo não leiam algo incompleto.
    ASSERT_OR(
        res = !update_header_status('0', fp),
        "Error: could not write status to file %s.",
        bin_fname
    );

    // Pula o header. Como nem todas as informações são conhecidas nesse
    // momento, primeiro processamos as linhas e depois voltamos para escrever o
    // header.
    fseek(fp, BUS_LINE_HEADER_SIZE, SEEK_SET);

    // Configura os argumentos do iterador. Esses valores serão modificados para
    // conter alguns dados que só podem ser contados lendo todos os registros.
    IterArgs args = {
        .fp                = fp,
        .reg_count         = 0,
        .removed_reg_count = 0,
    };

    // Itera por todas as linhas do csv e escreve os registros no binário.
    CSV_ASSERT(res = csv_iterate_rows(&csv, ",", (const IterFunc *)bus_line_row_iterator, &args));

    // Voltamos ao começo do arquivo para escrever as últimas coisas antes de
    // finalizar o processamento.
    fseek(fp, 0L, SEEK_SET);

    DBMeta meta = {
        .status          = '1',
        .byteProxReg     = ftell(fp),
        .nroRegistros    = args.reg_count,
        .nroRegRemovidos = args.removed_reg_count,
    };

    DBBusLineHeader header = { .meta = meta };

    memcpy(&header.descreveCodigo, csv_get_col_name(&csv, 0), sizeof(header.descreveCodigo));
    memcpy(&header.descreveCartao, csv_get_col_name(&csv, 1), sizeof(header.descreveCartao));
    memcpy(&header.descreveNome  , csv_get_col_name(&csv, 2), sizeof(header.descreveNome));
    memcpy(&header.descreveCor   , csv_get_col_name(&csv, 3), sizeof(header.descreveCor));

    ASSERT_OR(
        res = !write_bus_lines_header(header, fp),
        "Error: could not write bus lines header to file %s.",
        bin_fname
    );

teardown:
    // Libera os valores abertos/alocados.
    fclose(fp);
    csv_drop(csv);


    // Nesse momento, `res = 0` somente se não houve erro. Mas nesse caso
    // queremos retornar `true`.
    return !res;
}
