#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <common.h>
#include <parsing.h>
#include <csv.h>

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
