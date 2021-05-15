#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#include <csv.h>
#include <utils.h>

#define ASSERT_OK(val) \
    do { \
        CSVResult __tmp = (val); \
        if (__tmp != CSV_OK) return __tmp; \
    } while(0)

#define MIN_CAPACITY 8

// Static line buffer. This buffer will be only be reallocated a few times. It
// is reused for every line read which makes it very efficient. When every `CSV`
// object is dropped, so will the line buffer.
static char *line = NULL;
static size_t line_cap = 0;

// Counts the number of instances of `CSV` types.
static size_t instances_count = 0;

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

void csv_error(CSV *csv, const char *format, ...) {
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
    instances_count++;

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
        .fp         = NULL,
        .error_msg  = NULL,
    };
}

void csv_drop(CSV csv) {
    if (csv.fp)
        csv_close(&csv);

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

    // When all instances are deallocated, there is no reason to mantain the
    // allocation of the line. This ensures that if all `CSV` instances are
    // freed, then `line` will also be freed.
    if (--instances_count == 0) {
        free(line);
        line     = NULL;
        line_cap = 0;
    }
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
        fprintf(stderr, "csv_error: column index %zu is out of bounds for CSV.\n", col_idx);
        exit(1);
    }
    csv->columns[col_idx] = column;
}

static CSVResult csv_parse_value(CSV *csv, Column col, void *field, const char *input) {
    return col.parse(csv, input, field);
}

static CSVResult csv_parse_row_into(CSV *csv, char *input, const char *sep, void *row_values) {
    char *parse_field, *parse_ptr = input;

    int i = 0;
    CSVResult status = CSV_OK;

    while (parse_ptr != NULL && status == CSV_OK && i < csv->n_columns) {
        Column col = csv->columns[i];
        parse_field = strsep(&parse_ptr, sep);

        void *field = row_values + col.offset;

        status = csv_parse_value(csv, col, field, parse_field);

        if (status == CSV_OK) {
            i++;
            csv->curr_field++;
        }

    }

    if (parse_ptr != NULL && i == csv->n_columns && status == CSV_OK) {
        csv_error_curr(csv, "got more columns than expected");
        i = csv->n_columns;
        status = CSV_ERR_PARSE;
    }

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
    }
    return status;
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

    ASSERT_OK(csv_parse_row_into(csv, input, sep, csv->values + csv->n_rows * csv->elsize));

    csv->n_rows++;
    csv->curr_line++;
    csv->curr_field = 0;

    return CSV_OK;
}

CSVResult csv_open(CSV *csv, const char *fname) {
    if (csv->fp) {
        csv_error(csv, "can not open another file with same handler");
        return CSV_ERR_FILE;
    }

    FILE *fp = fopen(fname, "r");

    if (!fp) {
        csv_error(csv, "could not open file %s", fname);
        return CSV_ERR_FILE;
    }

    csv->fname = (char *)fname;
    csv->fp    = fp;

    return CSV_OK;
}

CSVResult csv_close(CSV *csv) {
    if (!csv->fp) {
        csv_error(csv, "tried to close csv with no file opened");
        return CSV_ERR_FILE;
    }
    fclose(csv->fp);

    csv->fp = NULL;
    csv->fname = NULL;

    return CSV_OK;
}

CSVResult csv_parse_header(CSV *csv, const char *sep) {
    if (!csv_is_open(csv)) {
        csv_error(csv, "tried to read line in csv with no file opened");
        return CSV_ERR_FILE;
    }

    if (getline(&line, &line_cap, csv->fp) == EOF)
        return CSV_ERR_EOF;

    strip_end(line, "\n\r");

    char *field, *parse_ptr = line;
    CSVResult status = CSV_OK;

    int i;
    for (i = 0; parse_ptr != NULL && status == CSV_OK && i < csv->n_columns; i++) {
        Column *col = &csv->columns[i];
        field = strsep(&parse_ptr, sep);

        if (!col->name) {
            col->name = strdup(field);
        } else if (strcmp(col->name, field) != 0){
            csv_error_curr(
                csv,
                "expected column name to be '%s', but found name '%s'",
                col->name,
                field
            );
            status = CSV_ERR_PARSE;
        }
    }

    if (parse_ptr != NULL && status == CSV_OK) {
        csv_error_curr(csv, "got more columns than expected");
        status = CSV_ERR_PARSE;
    }

    return status;
}

CSVResult csv_parse_next_row(CSV *csv, void *strct, const char *sep) {
    if (!csv_is_open(csv)) {
        csv_error(csv, "tried to read line in csv with no file opened");
        return CSV_ERR_FILE;
    }

    if (getline(&line, &line_cap, csv->fp) == EOF)
        return CSV_ERR_EOF;

    strip_end(line, "\n\r");
    return csv_parse_row_into(csv, line, sep, strct);
}

CSVResult csv_iterate_rows(CSV *csv, const char *sep, IterFunc *iter, void *arg) {
    // Using a VLA here might not be the best solution. But it should be faster
    // than a heap allocation. Has to be aligned to the biggest alignment so
    // that no words or types are broken in half.
    uint8_t strct[csv->elsize] __attribute__ ((aligned (__BIGGEST_ALIGNMENT__)));

    CSVResult status;
    do {
        status = csv_parse_next_row(csv, (void *)&strct, sep);
        if (status == CSV_OK) {
            status = iter(csv, (const void *)&strct, arg);
            free_row(csv, (void *)&strct);
        }
    } while (status == CSV_OK);

    if (status == CSV_OK || status == CSV_ERR_EOF)
        return CSV_OK;
    else
        return status;
}

CSVResult csv_parse_file(CSV *csv, const char *fname, const char *sep) {
    ASSERT_OK(csv_open(csv, fname));
    ASSERT_OK(csv_parse_header(csv, sep));

    while (getline(&line, &line_cap, csv->fp) != EOF) {
        strip_end(line, "\n\r");
        ASSERT_OK(csv_parse_row(csv, line, sep));
    }

    ASSERT_OK(csv_close(csv));
    return CSV_OK;
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

bool csv_is_open(CSV *csv) {
    return csv->fp != NULL;
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
