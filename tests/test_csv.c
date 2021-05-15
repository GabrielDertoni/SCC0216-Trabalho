#include <stdio.h>
#include <string.h>

#include <common.h>
#include <csv.h>

#define NULL_VAL "NULO"
/*
#define PASSERT(expr) if (!(expr)) return CSV_ERR_PARSE

#define IF_NULL_DEFAULT(input, field, default_val) \
    do { \
        if (!strcmp(input, NULL_VAL)) { \
            memcpy(field, default_val, sizeof(*field)); \
            return CSV_OK; \
        } \
    } while (0)

#define PASSERT_LEN(input, field) \
    do { \
        for (int i = 0; i < sizeof(*field); i++) \
            PASSERT(input[i]); \

        PASSERT(input[sizeof(*field)] == '\0');
    } while (0)

typedef struct {
    size_t size;
    char *default_val;
} StringInfo;

CSVResult parse_static_string(CSV *csv, const char *input, char *field, StringInfo *info) {
    if (!strcmp(input, NULL_VAL)) {
        memcpy(field, info->default_val, info->size);
        return CSV_OK;
    }

    for (int i = 0; i < info->size; i++)
        PASSERT(input[i]);

    PASSERT(input[info->size] == '\0');
    memcpy(field, input info->size);
    return CSV_OK;
}

#define DEF_CLOSURE(name, fn, info_type, __VA_ARGS__) \
    info_type name##_info = { __VA_ARGS__ }; \
    const ParseClosure name = { \
        .info = name##_info, \
        .fn   = fn, \
    }

typedef struct {
    void *info;
    ParseFunc *fn;
} ParseClosure;

const StringInfo vehicle_prefixo_info = {
    .size        = 5,
    .default_val = "\0@@@@",
};

const ParseClosure vehicle_parse_prefixo = {
    .info = &vehicle_prefixo_info,
    .fn   = parse_static_string,
};

DEF_CLOSURE(
    vehicle_parse_prefixo,
    parse_static_string,
    StringInfo,
        .size = 5,
        .default_val = "\0@@@@"
);

DEF_CLOSURE(
    vehicle_parse_data,
    parse_static_string,
    StringInfo,
        .size        = 10,
        .default_val = "\0@@@@@@@@@"
);
*/

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

static CSVResult parse_dynamic_string(CSV *csv, const char *input, char **field) {
    if (!strcmp(input, NULL_VAL)) {
        *field = NULL;
    } else {
        *field = strdup(input);
    }

    return CSV_OK;
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

CSVResult vehicle_parse_prefixo(CSV *csv, const char *input, char (*field)[5]) {
    return parse_static_string(csv, input, (char *)field, sizeof(*field), "\0@@@@");
}

CSVResult vehicle_parse_data(CSV *csv, const char *input, char (*field)[10]) {
    return parse_static_string(csv, input, (char *)field, sizeof(*field), "\0@@@@@@@@@");
}

CSV configure_vehicle_csv() {
    CSV csv = csv_new(sizeof(Vehicle), 6);

    csv_set_column(&csv, 0, csv_column(Vehicle, prefixo          , csv_static_field(vehicle_parse_prefixo)));
    csv_set_column(&csv, 1, csv_column(Vehicle, data             , csv_static_field(vehicle_parse_data)));
    csv_set_column(&csv, 2, csv_column(Vehicle, quantidadeLugares, csv_static_field(parse_i32)));
    csv_set_column(&csv, 3, csv_column(Vehicle, codLinha         , csv_static_field(parse_i32)));
    csv_set_column(&csv, 4, csv_column(Vehicle, modelo           , csv_dynamic_field(parse_dynamic_string)));
    csv_set_column(&csv, 5, csv_column(Vehicle, categoria        , csv_dynamic_field(parse_dynamic_string)));

    return csv;
}

int main() {

    CSV csv = configure_vehicle_csv();

    CSVResult res = csv_parse_file(&csv, "data/veiculo.csv", ",");

    if (res == CSV_ERR_FILE || res == CSV_ERR_PARSE) {
        fprintf(
            stderr,
            "%s: %s\n",
            res == CSV_ERR_PARSE ? "ParseError" : "FileError",
            csv.error_msg
        );
        csv_drop(csv);
        return 1;
    }

    Vehicle *vehicles = csv_get_values(&csv, Vehicle);

    // Linha 1: DN020,2002-12-18,18,560,MARCOPOLO SENIOR,MICRO
    int i = 45;
    printf("Vehicle 1:\n");
    printf("prefixo: %.*s\n", (int)sizeof(vehicles[i].prefixo), vehicles[i].prefixo);
    printf("data: %.*s\n", (int)sizeof(vehicles[i].data), vehicles[i].data);
    printf("quantidadeLugares: %d\n", vehicles[i].quantidadeLugares);
    printf("codLinha: %d\n", vehicles[i].codLinha);
    printf("modelo: %s\n", vehicles[i].modelo);
    printf("categoria: %s\n", vehicles[i].categoria);

    csv_drop(csv);
    return 0;
}
