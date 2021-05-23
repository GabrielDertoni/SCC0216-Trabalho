#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#include <csv.h>
#include <utils.h>

// Macro that asserts that a certain expression evaluates to `CSV_OK`. In case
// it does not, return the resulting value from the surrounding function.
#define ASSERT_OK(val) \
    do { \
        CSVResult __tmp = (val); \
        if (__tmp != CSV_OK) return __tmp; \
    } while(0)

// The minimum capacity that the register buffer can have.
#define MIN_CAPACITY 8

// Static line buffer. This buffer will be only be reallocated a few times. It
// is reused for every line read which makes it very efficient. It is freed when
// every `CSV` object is dropped.
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

// Removes from the end of `str` as many chars contained in `to_strip` as it can
// before it finds some char not int `to_strip`.
static void strip_end(char *str, const char *to_strip) {
    int i;
    for (i = strlen(str) - 1; strchr(to_strip, str[i]) && i > 0; i--);
    if (str[i] != '\0') str[i+1] = '\0';
}

// Encontra a primeira ocorrência de algum dos bytes de `sep` que não esteja
// entre aspas em `*parse_ptr`. Esse token é terminado sobreescrevendo o
// separador por '\0', e `*parse_ptr` é atualizado para apontar para após o
// token. Caso nenhum delimitador seja encontrado, o token é considerado como a
// string toda e `*parse_ptr` se torna NULL. Possui suporte para escaping de
// aspas duplas.
//
// Exemplos:
// ```c
// char my_fields[] = "\"hello, world\" \"bye\\\" bye\\\"\",123";
// char *parse_ptr = my_fields;
//
// printf("%s\n", fieldsep(&parse_ptr, " ,")); // "hello, world"
// printf("%s\n", fieldsep(&parse_ptr, " ,")); // "bye\" bye\""
// printf("%s\n", fieldsep(&parse_ptr, " ,")); // 123
// ```
static char *fieldsep(char **parse_ptr, const char *sep) {
    char *ptr = *parse_ptr;
    bool in_quoted = false;

    while (!strchr(sep, **parse_ptr) || in_quoted) {
        // Skip escaped quote.
        if      (**parse_ptr == '\\') (*parse_ptr)++;
        else if (**parse_ptr == '"') in_quoted = !in_quoted;
        (*parse_ptr)++;
    }

    if (**parse_ptr == '\0') {
        *parse_ptr = NULL;
    } else {
        **parse_ptr = '\0';
        (*parse_ptr)++;
    }

    return ptr;
}

// Set csv `error_msg` to some format with variable arguments list.
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
    // allocation of `line`. This ensures that if all `CSV` instances are freed,
    // then `line` will also be freed.
    if (--instances_count == 0) {
        if (line) free(line);
        line     = NULL;
        line_cap = 0;
    }
}

void csv_set_column(CSV *csv, size_t col_idx, Column column) {
    if (col_idx >= csv->n_columns) {
        fprintf(stderr, "csv_error: column index %zu is out of bounds for CSV.\n", col_idx);
        exit(1);
    }
    csv->columns[col_idx] = column;
}

// Parses a single row of `csv` from `input` using `sep` as a field separator.
// Writes in the `row_values` buffer.
static CSVResult csv_parse_row_into(CSV *csv, char *input, const char *sep, void *row_values) {
    char *parse_field, *parse_ptr = input;

    int i = 0;
    CSVResult status = CSV_OK;

    while (parse_ptr != NULL && status == CSV_OK && i < csv->n_columns) {
        Column col = csv->columns[i];
        parse_field = fieldsep(&parse_ptr, sep);

        void *field = row_values + col.offset;

        status = col.parse(csv, parse_field, field);

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

// Parses a row of `csv` from `input` with field separator `sep`. Writes the
// result into the buffer of `csv` expanding the buffer if necessary.
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

CSVResult csv_use_fp(CSV *csv, FILE *fp) {
    if (csv->fp) {
        csv_error(csv, "another file pointer is already in use");
        return CSV_ERR_FILE;
    }

    csv->fp = fp;
    csv->fname = "unknown";

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

    if (csv->fp != stdin && csv->fp != stdout && csv->fp != stderr)
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

        field = fieldsep(&parse_ptr, sep);

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
    // Using a VLA here might not be the best solution, but it should be faster
    // than a heap allocation. Has to be aligned to the biggest alignment so
    // that no words or types are broken in half.
    uint8_t strct[csv->elsize] __attribute__ ((aligned (__BIGGEST_ALIGNMENT__)));

    CSVResult status;
    do {
        status = csv_parse_next_row(csv, (void *)&strct, sep);
        if (status == CSV_OK) {
            status = iter(csv, (const void *)&strct, arg);
            free_row(csv, (void *)&strct);
            csv->curr_line++;
            csv->curr_field = 0;
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

const char *csv_get_error(const CSV *csv) {
    return csv->error_msg;
}

const char *csv_get_fname(const CSV *csv) {
    return csv->fname;
}

size_t csv_get_curr_field(const CSV *csv) {
    return csv->curr_field;
}

size_t csv_get_curr_line(const CSV *csv) {
    return csv->curr_line;
}

bool csv_is_open(const CSV *csv) {
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

const char *csv_get_col_name(const CSV *csv, int idx) {
    return csv->columns[idx].name;
}

size_t csv_row_count(const CSV *csv) { return csv->n_rows; }

Column csv_column_new(
    size_t size,
    size_t offset,
    const char *name,
    ParseFunc *parse,
    DropFunc *drop
) {
    return (Column) {
        .size   = size,
        .offset = offset,
        .name   = name ? strdup(name) : NULL,
        .parse  = parse,
        .drop   = drop,
    };
}
