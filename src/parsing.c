#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <common.h>
#include <parsing.h>
#include <csv.h>
#include <bin.h>

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
    CSV csv = csv_new(sizeof(Vehicle), 6);

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

CSV configure_bus_line_csv() {
    CSV csv = csv_new(sizeof(BusLine), 4);

    csv_set_column(&csv, 0, csv_column(BusLine, codLinha    , csv_static_field(parse_i32)));
    csv_set_column(&csv, 1, csv_column(BusLine, aceitaCartao, csv_static_field(bus_line_parse_aceitaCartao)));
    csv_set_column(&csv, 2, csv_column(BusLine, nomeLinha   , csv_dynamic_field(parse_dynamic_string)));
    csv_set_column(&csv, 3, csv_column(BusLine, corLinha    , csv_dynamic_field(parse_dynamic_string)));

    return csv;
}

CSVResult vehicle_row_iterator(CSV *csv, const Vehicle *vehicle, VehicleIterArgs *args) {
    if (!write_vehicle(*vehicle, args->fp)) {
        csv_error(csv, "failed to write vehicle");
        return CSV_ERR_OTHER;
    } else {
        if (vehicle->prefixo[0] == REMOVED_MARKER)
            args->removed_reg_count++;
        else
            args->reg_count++;

        return CSV_OK;
    }
}
