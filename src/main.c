#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <common.h>
#include <csv.h>
#include <bin.h>

#define NULL_VAL "NULO"

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

/*
CSV configure_bus_line_csv() {
    CSV csv = csv_new(sizeof(BusLine), 4);
    csv_set_column(&csv, 0, csv_column_default(i32 , BusLine, codLinha    , -1));
    csv_set_column(&csv, 1, csv_column_default(char, BusLine, aceitaCartao, '\0'));
    csv_set_column(&csv, 2, csv_column_default(i32 , BusLine, nomeLinha   , -1));
    csv_set_column(&csv, 3, csv_column_default(i32 , BusLine, corLinha    , -1));
    return csv;
}
*/

int main(int argc, char *argv[]) {
    CSV vehicles_csv = configure_vehicle_csv();

    CSVResult res = csv_parse_file(&vehicles_csv, "data/veiculo.csv", ",");

    if (CSV_IS_ERROR(res)) {
        if (res == CSV_ERR_PARSE)
            fprintf(stderr, "ParseError: %s.\n", vehicles_csv.error_msg);
        else
            fprintf(stderr, "FileError: %s.\n", vehicles_csv.error_msg);

        csv_drop(vehicles_csv);
        return 1;
    }

    Vehicle *vehicles = csv_get_values(&vehicles_csv, Vehicle);

    printf("Vehicle 1:\n");
    printf("prefixo: %.*s\n", 5, vehicles[0].prefixo);
    printf("data: %.*s\n", 10, vehicles[0].data);
    printf("quantidadeLugares: %d\n", vehicles[0].quantidadeLugares);
    printf("codLinha: %d\n", vehicles[0].codLinha);
    printf("modelo: %s\n", vehicles[0].modelo);
    printf("categoria: %s\n", vehicles[0].categoria);
    printf("\n");

    uint32_t n_vehicles = csv_row_count(&vehicles_csv);

    write_vehicles(vehicles, n_vehicles, "out.bin");

    csv_drop(vehicles_csv);

    return 0;
}
