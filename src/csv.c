#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#include <csv.h>
#include <utils.h>

#define MIN_CAPACITY 8

static char *type_names[] = {
    [TYPE_I32]  = "i32",
    [TYPE_F64]  = "f64",
    [TYPE_CHAR] = "char",
    [TYPE_STR]  = "str",
    [TYPE_ANY]  = "?",
    [TYPE_NONE] = "-",
};

static uint8_t type_sizes[] = {
    [TYPE_I32]  = 4,
    [TYPE_F64]  = 8,
    [TYPE_CHAR] = sizeof(char),
    [TYPE_STR]  = sizeof(char *),
    [TYPE_ANY]  = sizeof(void *),
    [TYPE_NONE] = 0,
};

uint8_t csv_sizeof_type(PrimitiveType type) {
    return type_sizes[type];
}

static void free_value(PrimitiveType type, Value *value) {
    if (type == TYPE_ANY || type == TYPE_STR)
        free(value->any);
}

static void free_row(CSV *csv, void *value) {
    for (int i = 0; i < csv->n_columns; i++) {
        Column col = csv->columns[i];
        free_value(col.type, (Value *)(value + col.offset));
    }
}

static char *skip_whitespace(const char *str) {
    int i;
    for (i = 0; isspace(str[i]); i++);
    return (char *)(str + i);
}

static void strip_end(char *str, const char *to_strip) {
    int i;
    for (i = strlen(str) - 1; strchr(to_strip, str[i]) && i > 0; i--);
    if (str[i] != '\0') str[i+1] = '\0';
}

static void error(CSV *csv, const char *format, ...) {
    va_list ap;
    va_start(ap, format);

    if (csv->error_msg) free(csv->error_msg);
    csv->error_msg = alloc_vsprintf(format, ap);

    va_end(ap);
}

CSV csv_new(size_t elsize, size_t n_columns) {
    return (CSV) {
        .columns   = (Column *)calloc(n_columns, sizeof(Column)),
        .n_columns = n_columns,
        .n_rows    = 0,
        .capacity  = 0,
        .elsize    = elsize,
        .values    = NULL,
        .error_msg = NULL,

        .allow_partial_field_as_null = false,
    };
}

void csv_drop(CSV csv) {
    for (int i = 0; i < csv.n_rows; i++)
        free_row(&csv, csv.values + i * csv.elsize);

    if (csv.values)
        free(csv.values);

    for (int i = 0; i < csv.n_columns; i++)
	    if (csv.columns[i].name)
            free(csv.columns[i].name);

    free(csv.columns);

    if (csv.error_msg)
        free(csv.error_msg);
}

void csv_set_column(CSV *csv, size_t col_idx, Column column) {
    if (col_idx >= csv->n_columns) {
        fprintf(stderr, "Error: column index %zu is out of bounds for CSV.\n", col_idx);
        exit(1);
    }
    csv->columns[col_idx] = column;
}

void csv_set_flag(CSV *csv, CSVFlag flag, bool val) {
    switch (flag) {
        case CSV_FLAG_PARTIAL_FIELD_AS_NULL:
            csv->allow_partial_field_as_null = val;
            break;
    }
}

static CSVResult csv_parse_char(
    CSV *csv,
    Column col,
    char *field,
    const char *input,
    char **endptr
) {
    int i;
    for (i = 0; i < col.size && input[i]; i++) {
        field[i] = input[i];
    }

    bool is_empty = i == 0;
    bool is_partial = !is_empty && i < col.size;

    if (!col.is_required && (is_empty || (is_partial && csv->allow_partial_field_as_null))) {
        memcpy(field, col.default_val.str, col.size);
        return CSV_OK;
    }

    if (is_partial || is_empty) {
        error(csv,
            "%s field not allowed in field \"%s\" on row %d. "
            "Expected it to be %ld chars long but was only %d chars long "
            "with value \"%.*s\"",
            is_partial ? "partial" : "empty",
            col.name,
            csv->n_rows + 1,
            col.size,
            i,
            i,
            input
        );
        return CSV_ERR_PARSE;
    }
    *endptr = (char *)input + i;
    return CSV_OK;
}

static CSVResult csv_parse_value(CSV *csv, Column col, Value *field, const char *input) {
    char *endptr = NULL;

    switch (col.type) {
        case TYPE_I32:  field->i32 = strtol(input, &endptr, 10); break;
        case TYPE_F64:  field->f64 = strtod(input, &endptr);     break;
        case TYPE_STR:  field->str = strdup(input);              break;
        case TYPE_ANY:  field->any = (void *)strdup(input);      break;
        case TYPE_NONE: endptr = (char *)input;                  break;
        case TYPE_CHAR:
            return csv_parse_char(csv, col, &field->chr, input, &endptr);
    }

    bool is_field_null = false;

    if ((col.type == TYPE_I32 || col.type == TYPE_F64) && endptr == input) {
        is_field_null = true;
    }

    if (endptr) {
        endptr = skip_whitespace(endptr);

        if (*endptr != '\0') {
            is_field_null = true;
        }
    }

    // TODO: Add partial field handling.
    if (is_field_null) {
        if (col.is_required) {
            error(csv,
                "field \"%s\" of type %s is required but not provided or of wrong type in row %d.",
                col.name,
                type_to_string(col.type),
                csv->n_rows + 1
            );
            return CSV_ERR_PARSE;
        } else {
            if (col.type == TYPE_STR) {
                field->str = strdup(col.default_val.str);
            } else {
                *field = col.default_val;
            }
        }
    }

    return CSV_OK;
}

static CSVResult csv_parse_row(CSV *csv, char *input, const char *sep) {
    // Dynamically allocate memory for the rows.
    if (csv->n_rows >= csv->capacity) {

        if (csv->capacity == 0) {
            csv->capacity = MIN_CAPACITY;
        } else {
            csv->capacity *= 2;
        }

        csv->values = (void *)realloc(csv->values, csv->capacity * csv->elsize);
    }

    char *parse_field, *parse_ptr = input;
    void *row_values = csv->values + csv->n_rows * csv->elsize;

    int i = 0;
    CSVResult status;
    do {
        if (i >= csv->n_columns) {
            csv->error_msg = strdup("got more columns than expected");
            return CSV_ERR_PARSE;
        }

        Column col = csv->columns[i++];
        parse_field = strsep(&parse_ptr, sep);

        void *field = row_values + col.offset;

        status = csv_parse_value(csv, col, (Value *)field, parse_field);
    } while (parse_ptr != NULL && status == CSV_OK);

    if (i < csv->n_columns && status == CSV_OK) {
        error(csv, "got fewer columns than expected on row %d", csv->n_rows + 1);
        status = CSV_ERR_PARSE;
    }

    if (status != CSV_OK) {
        for (int j = 0; j < i; j++) {
            Column col = csv->columns[j];
            free_value(col.type, (Value *)(row_values + col.offset));
        }

        return status;
    }

    csv->n_rows++;

    return CSV_OK;
}

CSVResult csv_parse_file(CSV *csv, const char *fname, const char *sep) {
    FILE *fp = fopen(fname, "r");

    if (!fp) {
        error(csv, "could not open file");
        return CSV_ERR_FILE;
    }

    char *line = NULL;
    size_t line_cap = 0;

    if (getline(&line, &line_cap, fp) == EOF) return CSV_OK;

    strip_end(line, "\n\r");

    char *parse_ptr = line;
    char *field;
    int i = 0;
    CSVResult ret = CSV_OK;

    do {
        Column *col = &csv->columns[i];
        field = strsep(&parse_ptr, sep);

        if (!col->name) {
            col->name = strdup(field);
        } else if (strcmp(col->name, field) != 0){
            fprintf(stderr,
                    "Warning: column %d expected name '%s' but found name '%s'\n",
                    i, col->name, field);
        }

        i++;
    } while (parse_ptr != NULL);

    while (getline(&line, &line_cap, fp) != EOF) {
        strip_end(line, "\n\r");
        ret = csv_parse_row(csv, line, sep);
        if (ret == CSV_ERR_PARSE) break;
    }

    if (line_cap > 0)
        free(line);

    fclose(fp);
    return ret;
}

char *type_to_string(PrimitiveType type) {
    return type_names[type];
}

void csv_print_header(CSV *csv) {
    if (csv->n_columns == 0) {
        printf("No headers...\n");
        return;
    }

    Column col = csv->columns[0];
    printf("%s (%s)",
           col.name ? col.name : "[undefined]",
           type_to_string(col.type));

    for (int i = 1; i < csv->n_columns; i++) {
        col = csv->columns[i];
        printf(" | %s (%s)",
               col.name ? col.name : "[undefined]",
               type_to_string(col.type));
    }

    printf("\n");
}

void *csv_get_raw_values(const CSV *csv) {
    return csv->values;
}

int csv_get_field_index(const CSV *csv, const char *field_name) {
    int i;
    for (i = 0; i < csv->n_rows && strcmp(field_name, csv->columns[i].name) != 0; i++);
    if (i == csv->n_rows) return -1;
    return i;
}

size_t csv_row_count(const CSV *csv) { return csv->n_rows; }

Column csv_column_new(PrimitiveType type, size_t size, size_t offset, const char *name) {
    return (Column) {
        .type        = type,
        .size        = size,
        .offset      = offset,
        .name        = name ? strdup(name) : NULL,
        .is_required = true,
        .default_val = (Value)NULL,
    };
}

Column csv_column_new_with_default(PrimitiveType type, size_t size, size_t offset, const char *name, Value default_val) {
    return (Column) {
        .type        = type,
        .size        = size,
        .offset      = offset,
        .name        = name ? strdup(name) : NULL,
        .is_required = false,
        .default_val = default_val,
    };
}
