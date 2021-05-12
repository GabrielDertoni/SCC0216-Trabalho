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

static void free_row(CSV *csv, void *value) {
    for (int i = 0; i < csv->n_columns; i++) {
        Column col = csv->columns[i];

        if (col.drop)
            col.drop(*(void **)(value + col.offset));
    }
}

static void strip_end(char *str, const char *to_strip) {
    int i;
    for (i = strlen(str) - 1; strchr(to_strip, str[i]) && i > 0; i--);
    if (str[i] != '\0') str[i+1] = '\0';
}

static void verror(CSV *csv, const char *format, va_list ap) {
    if (csv->error_msg) free(csv->error_msg);
    csv->error_msg = alloc_vsprintf(format, ap);
}

static void error(CSV *csv, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    verror(csv, format, ap);
    va_end(ap);
}

void csv_error_curr(CSV *csv, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    
    char *new_format = alloc_sprintf(
        "%s (at %s:%d in field %d)",
        format,
        csv_get_fname(csv),
        csv_get_curr_line(csv)  + 1,
        csv_get_curr_field(csv) + 1
    );

    verror(csv, new_format, ap);

    free(new_format);
    va_end(ap);
}

CSV csv_new(size_t elsize, size_t n_columns) {
    return (CSV) {
        .columns    = (Column *)calloc(n_columns, sizeof(Column)),
        .n_columns  = n_columns,
        .curr_line  = 0,
        .curr_field = 0,
        .n_rows     = 0,
        .capacity   = 0,
        .elsize     = elsize,
        .values     = NULL,
        .fname      = NULL,
        .error_msg  = NULL,
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

char *csv_get_fname(CSV *csv) {
    return csv->fname;
}

size_t csv_get_curr_field(CSV *csv) {
    return csv->curr_field;
}

size_t csv_get_curr_line(CSV *csv) {
    return csv->curr_line;
}

void csv_set_column(CSV *csv, size_t col_idx, Column column) {
    if (col_idx >= csv->n_columns) {
        fprintf(stderr, "Error: column index %zu is out of bounds for CSV.\n", col_idx);
        exit(1);
    }
    csv->columns[col_idx] = column;
}

static CSVResult csv_parse_value(CSV *csv, Column col, void *field, const char *input) {
    return col.parse(csv, input, field);
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
            csv_error_curr(csv, "got more columns than expected");
            i = csv->n_columns;
            status = CSV_ERR_PARSE;
            break;
        }

        Column col = csv->columns[i];
        parse_field = strsep(&parse_ptr, sep);

        void *field = row_values + col.offset;

        status = csv_parse_value(csv, col, field, parse_field);

        if (status == CSV_OK) {
            i++;
            csv->curr_field++;
        }

    } while (parse_ptr != NULL && status == CSV_OK);

    if (i < csv->n_columns && status == CSV_OK) {
        csv_error_curr(csv, "got fewer columns than expected");
        status = CSV_ERR_PARSE;
    }

    if (status != CSV_OK) {
        for (int j = 0; j < i; j++) {
            Column col = csv->columns[j];

            if (col.drop) col.drop(*(void **)(row_values + col.offset));
        }
        csv->curr_field = 0;
        return status;
    }

    csv->n_rows++;
    csv->curr_line++;
    csv->curr_field = 0;

    return CSV_OK;
}

CSVResult csv_parse_file(CSV *csv, const char *fname, const char *sep) {
    FILE *fp = fopen(fname, "r");

    if (!fp) {
        error(csv, "could not open file %s", fname);
        return CSV_ERR_FILE;
    }

    csv->fname = (char *)fname;

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
            csv_error_curr(
                csv,
                "expected column name was '%s', but found name '%s'",
                col->name,
                field
            );
            return CSV_ERR_PARSE;
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

    csv->fname = NULL;
    fclose(fp);
    return ret;
}

void csv_print_header(CSV *csv) {
    if (csv->n_columns == 0) {
        printf("No headers...\n");
        return;
    }

    Column col = csv->columns[0];
    printf("%s", col.name ? col.name : "[undefined]");

    for (int i = 1; i < csv->n_columns; i++) {
        col = csv->columns[i];
        printf(" | %s", col.name ? col.name : "[undefined]");
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

Column csv_column_new(size_t size, size_t offset, const char *name, ParseFunc *parse, DropFunc *drop) {
    return (Column) {
        .size   = size,
        .offset = offset,
        .name   = name ? strdup(name) : NULL,
        .parse  = parse,
        .drop   = drop,
    };
}

void heap_drop(void *ptr) {
    free(ptr);
}
