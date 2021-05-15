#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <common.h>
#include <parsing.h>
#include <csv.h>
#include <bin.h>

void print_error(CSV *csv) {
    fprintf(stderr, "Error: %s.\n", csv->error_msg);
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

static CSVResult bus_line_parse_codLinha(CSV *csv, const char *input, char (*field)[32]) {
    return parse_static_dynamic_string(csv, input, (char *)field, sizeof(*field), "-1\0");
}

CSV configure_bus_line_csv() {
    CSV csv = csv_new(sizeof(BusLine), 4);

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

#define CSV_ASSERT(expr)          \
    do {                          \
        if (CSV_IS_ERROR(expr)) { \
            print_error(&csv);    \
            goto teardown;        \
        }                         \
    } while (0);

#define WASSERT(expr) \
    if ((expr) != 0) \
        goto teardown

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

    char status = '0';
    WASSERT(res = !fwrite(&status, sizeof(char), 1, fp));
    fseek(fp, VEHICLE_HEADER_SIZE, SEEK_SET);

    IterArgs args = {
        .fp                = fp,
        .reg_count         = 0,
        .removed_reg_count = 0,
    };

    CSV_ASSERT(res = csv_iterate_rows(&csv, ",", (IterFunc *)vehicle_row_iterator, &args));

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

    fseek(fp, 0L, SEEK_SET);
    WASSERT(res = !write_vehicles_header(header, fp));

teardown:
    fclose(fp);
    csv_drop(csv);

    return !CSV_IS_ERROR(res);
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

    char status = '0';
    WASSERT(res = !fwrite(&status, sizeof(char), 1, fp));
    fseek(fp, BUS_LINE_HEADER_SIZE, SEEK_SET);

    IterArgs args = {
        .fp                = fp,
        .reg_count         = 0,
        .removed_reg_count = 0,
    };

    CSV_ASSERT(res = csv_iterate_rows(&csv, ",", (IterFunc *)bus_line_row_iterator, &args));

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

    fseek(fp, 0L, SEEK_SET);
    WASSERT(res = !write_bus_lines_header(header, fp));

teardown:
    fclose(fp);
    csv_drop(csv);

    return !CSV_IS_ERROR(res);
}

#undef CSV_ASSERT
