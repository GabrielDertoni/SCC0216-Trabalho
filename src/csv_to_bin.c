#include <string.h>

#include <csv.h>
#include <parsing.h>
#include <bin.h>

// Tipo que contém os argumentos adicionais para as funções iteradoras.
typedef struct {
    FILE *fp;
    size_t reg_count;
    size_t removed_reg_count;
} IterArgs;


// Função que será executada para cada linha do `csv` para veículos.
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

// Função que será executada para cada linha do `csv` para linhas de ônibus.
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
// Nos macros `do while` só é utilizado para que possamos escrever um ';' depois
// da utilização do macro.

#ifdef DEBUG
// Verifica se um determinada expressão é verdadeira. Se não for, imprime uma
// mensagem de erro fornecida e vai para o label `teardown`.
#define ASSERT(expr, ...)                 \
    do {                                  \
        if (!(expr)) {                    \
            fprintf(stderr, __VA_ARGS__); \
            goto teardown;                \
        }                                 \
    } while (0)
#else
// Verifica se um determinada expressão é verdadeira. Se não for, imprime uma
// mensagem de erro sempre igual e vai para o label `teardown`.
#define ASSERT(expr, ...)        \
    do {                         \
        if (!(expr)) {           \
            printf(ERROR_FOUND); \
            goto teardown;       \
        }                        \
    } while (0)
#endif

// Converte um csv para um arquivo binário de registros. A leitura do csv é
// controlada por `csv` e a escrita no binário é controlada por `iter`. O
// argumento `iter` tem que ser `vehicle_from_stdin_append_to_bin` ou
// `bus_line_row_iterator`, por conta dessa restrição essa não é uma função
// completamente genérica. De forma mais geral, `iter` pode ser qualquer função
// se encaixe nas restrições de `InterFunc` e receba como terceiro argumento um
// ponteiro do tipo `IterArgs`.
//
// Essa função assume que o csv possui um header ainda por ser lido. Além disso,
// os nomes das colunas são escritos em ordem e sem delimitador logo após o meta
// header no arquivo binário. Ou seja, o leitor do arquivo binário gerado tem
// que saber de antemão o tamanho do nome de cada coluna.
static bool csv_to_bin(
    CSV *csv,
    const char *bin_fname,
    CSVIterFunc *iter,
    const char *sep
) {
    bool ok = true;

    FILE *fp = fopen(bin_fname, "w");

    if (!fp) {
#ifdef DEBUG
        fprintf(stderr, "Error: Could not open file.\n");
#else
        printf(ERROR_FOUND);
#endif
        return false;
    }

    ASSERT(ok = csv_parse_header(csv, sep) == CSV_OK,
           "Error: %s.\n", csv_get_error(csv));

    // Primeira escrita, escreve o byte de status para garantir que outros
    // processos tentando ler o arquivo não leiam algo incompleto.
    ASSERT(ok = update_header_status('0', fp),
           "Error: could not write status to file %s.\n", bin_fname);

    // Pula o meta header. Como nem todas as informações são conhecidas nesse
    // momento, primeiro processamos as linhas e depois voltamos para escrever o
    // header.
    fseek(fp, META_SIZE, SEEK_SET);

    // Escreve os nomes das colunas em sequência sem \0 e sem verificar tamanho.
    // Portanto erros nesses nomes não serão detectados.
    for (int i = 0; i < csv->n_columns; i++) {
        const char *name = csv_get_col_name(csv, i);
        size_t len = strlen(name);

        ASSERT(ok = fwrite(name, len * sizeof(char), 1, fp),
               "Error: could not write column name to file %s.\n", bin_fname);
    }

    // Configura os argumentos do iterador. Esses valores serão modificados para
    // conter alguns dados que só podem ser contados lendo todos os registros.
    IterArgs args = {
        .fp                = fp,
        .reg_count         = 0,
        .removed_reg_count = 0,
    };

    // Itera por todas as linhas do csv e escreve os registros no binário.
    ASSERT(ok = csv_iterate_rows(csv, sep, iter, &args) == CSV_OK,
           "Error: %s.\n", csv_get_error(csv));

    DBMeta meta = {
        .status          = '1',
        .byteProxReg     = ftell(fp),
        .nroRegistros    = args.reg_count,
        .nroRegRemovidos = args.removed_reg_count,
    };

    // Volta e atualiza o meta header.
    ASSERT(ok = update_header_meta(&meta, fp),
           "Error: could not write the meta header to file %s.\n", bin_fname);

teardown:
    // Libera os valores abertos/alocados.
    fclose(fp);

    return ok;
}

// Converte um csv de veículos para um binário contendo os registros. Retorna
// `true` caso a operação tenha sido bem sucedida e `false` caso contrário.
bool vehicle_csv_to_bin(const char *csv_fname, const char *bin_fname) {
    CSV csv = configure_vehicle_csv();
    bool ok = csv_open(&csv, csv_fname) == CSV_OK;

    if (ok) {
        ok = csv_to_bin(&csv, bin_fname, (CSVIterFunc *)vehicle_row_iterator, ",");
    } else {
#ifdef DEBUG
        fprintf(stderr, "Error: %s.\n", csv_get_error(&csv));
#else
        printf(ERROR_FOUND);
#endif
    }

    csv_drop(csv);
    return ok;
}

// Converte um csv de linhas de ônibus para um binário contendo os registros.
// Retorna `true` caso a operação tenha sido bem sucedida e `false` caso
// contrário.
bool bus_line_csv_to_bin(const char *csv_fname, const char *bin_fname) {
    CSV csv = configure_bus_line_csv();
    bool ok = csv_open(&csv, csv_fname) == CSV_OK;

    if (ok) {
        ok = csv_to_bin(&csv, bin_fname, (CSVIterFunc *)bus_line_row_iterator, ",");
    } else {
#ifdef DEBUG
        fprintf(stderr, "Error: %s.\n", csv_get_error(&csv));
#else
        printf(ERROR_FOUND);
#endif
    }

    csv_drop(csv);
    return ok;
}

// Lê as linhas de um csv com campos separados por `sep` e escreve os registros
// lidos no arquivo binário de nome `bin_fname`. A função `iter` tem que ser
// `vehicle_row_iterator` ou `bus_line_row_iterator`.
//
// Considera que não há header no csv.
static bool csv_append_to_bin(const char *bin_fname, CSV *csv, CSVIterFunc *iter, const char *sep) {
    FILE *fp = fopen(bin_fname, "r+b");

    if (!fp) {
#ifdef DEBUG
        fprintf(stderr, "Error: Could not open file.\n");
#else
        printf(ERROR_FOUND);
#endif
        return false;
    }

    bool ok = true;
    DBMeta meta;

    ASSERT(ok = read_meta(fp, &meta),
           "Error: could not read meta header from file '%s'.\n", bin_fname);

    ASSERT(ok = update_header_status('0', fp),
           "Error: could not write status to file '%s'.\n", bin_fname);

    IterArgs args = {
        .fp                = fp,
        .reg_count         = 0,
        .removed_reg_count = 0,
    };

    // Vai para o fim do arquivo para adicionar novos registros.
    fseek(fp, 0L, SEEK_END);

    ASSERT(ok = csv_iterate_rows(csv, sep, (CSVIterFunc *)iter, &args) == CSV_OK,
           "Error: %s.\n", csv_get_error(csv));

    meta.status = '1';
    meta.byteProxReg = ftell(fp);
    meta.nroRegRemovidos += args.removed_reg_count;
    meta.nroRegistros += args.reg_count;

    ASSERT(ok = update_header_meta(&meta, fp),
           "Error: could not write meta header to file '%s'.\n", bin_fname);

teardown:
    fclose(fp);

    return ok;
}

// Lê da entrada padrão vários registros de veículos e escreve esses registros
// no arquivo `bin_fname`.
bool vehicle_append_to_bin_from_stdin(const char *bin_fname) {
    CSV csv = configure_vehicle_csv();
    csv_use_fp(&csv, stdin);

    bool ok = csv_append_to_bin(bin_fname, &csv, (CSVIterFunc *)vehicle_row_iterator, " ");

    csv_drop(csv);
    return ok;
}

// Lê da entrada padrão vários registros de linhas de ônibus e escreve esses
// registros no arquivo `bin_fname`.
bool bus_line_append_to_bin_from_stdin(const char *bin_fname) {
    CSV csv = configure_bus_line_csv();
    csv_use_fp(&csv, stdin);

    bool ok = csv_append_to_bin(bin_fname, &csv, (CSVIterFunc *)bus_line_row_iterator, " ");

    csv_drop(csv);
    return ok;
}
