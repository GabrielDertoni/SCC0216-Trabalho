#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <external.h>
#include <index.h>
#include <btree.h>
#include <bin.h>
#include <csv.h>
#include <parsing.h>
#include <common.h>

#define ASSERT(expr) \
    if (!(expr)) \
        goto teardown

bool index_vehicle_create(const char *bin_fname, const char *index_fname) {
    BTreeMap btree = btree_new();

    bool ok = true;
    FILE *bin_fp = fopen(bin_fname, "rb");

    ASSERT(bin_fp);
    ASSERT(ok = btree_create(&btree, index_fname) == BTREE_OK);

    DBVehicleHeader header;
    ASSERT(ok = read_header_vehicle(bin_fp, &header));

    DBVehicleRegister reg;

    // TODO: Por algum motivo usar `total_registers` não funciona. Talvez um
    //       erro com o caso de teste?
    // uint32_t total_register = header.meta.nroRegistros + header.meta.nroRegRemovidos;

    uint64_t offset = ftell(bin_fp);

    for (int i = 0; read_vehicle_register(bin_fp, &reg); i++){

        if (reg.removido == '1') {
            int32_t hash = convertePrefixo(reg.prefixo);
            ASSERT(ok = btree_insert(&btree, hash, offset) == BTREE_OK);
        }

        free(reg.modelo);
        free(reg.categoria);

        offset = ftell(bin_fp);
    }

teardown:

    if (!ok) {
#ifdef DEBUG
        if (btree.error_msg) {
            fprintf(stderr, "Error: %s\n", btree.error_msg);
        } else {
            fprintf(stderr, "Error: unexpected\n");
        }
#else
        printf(ERROR_FOUND);
#endif
    }

    btree_drop(btree);
    fclose(bin_fp);
    return ok;
}

bool index_bus_line_create(const char *bin_fname, const char *index_fname) {
    BTreeMap btree = btree_new();

    bool ok = true;
    FILE *bin_fp = fopen(bin_fname, "rb");

    ASSERT(bin_fp);
    ASSERT(ok = btree_create(&btree, index_fname) == BTREE_OK);

    DBBusLineHeader header;
    ASSERT(ok = read_header_bus_line(bin_fp, &header));

    DBBusLineRegister reg;

    // uint32_t total_register = header.meta.nroRegistros + header.meta.nroRegRemovidos;
    uint64_t offset = ftell(bin_fp);

    for (int i = 0; read_bus_line_register(bin_fp, &reg); i++){
        if (reg.removido == '1') {
            ASSERT(ok = btree_insert(&btree, reg.codLinha, offset) == BTREE_OK);
        }

        free(reg.nomeLinha);
        free(reg.corLinha);

        offset = ftell(bin_fp);
    }

teardown:

    if (!ok) {
#ifdef DEBUG
        if (btree.error_msg) {
            fprintf(stderr, "Error: %s\n", btree.error_msg);
        } else {
            fprintf(stderr, "Error: unexpected\n");
        }
#else
        printf(ERROR_FOUND);
#endif
    }

    btree_drop(btree);
    fclose(bin_fp);
    return ok;
}

bool search_for_vehicle(const char *bin_fname, const char *index_fname, const char prefixo[6]) {
    BTreeMap btree = btree_new();

    bool ok = true;
    FILE *bin_fp = fopen(bin_fname, "rb");

    ASSERT(bin_fp);

    DBVehicleHeader header;
    ASSERT(ok = read_header_vehicle(bin_fp, &header));
    ASSERT(ok = btree_load(&btree, index_fname) == BTREE_OK);

    int32_t hash = convertePrefixo((char *)prefixo);
    int64_t off = btree_get(&btree, hash);
    
    ASSERT(!btree_has_error(&btree));

    if (off < 0) {
        printf("Registro inexistente.\n");
    } else {
        fseek(bin_fp, off, SEEK_SET);

        DBVehicleRegister reg;
        ASSERT(ok = read_vehicle_register(bin_fp, &reg));
        print_vehicle(stdout, &reg, &header);
    }

teardown:

    if (!ok) {
#ifdef DEBUG
        if (btree.error_msg) {
            fprintf(stderr, "Error: %s\n", btree.error_msg);
        } else {
            fprintf(stderr, "Error: unexpected\n");
        }
#else
        printf(ERROR_FOUND);
#endif
    }

    btree_drop(btree);
    fclose(bin_fp);
    return true;
}

bool search_for_bus_line(const char *bin_fname, const char *index_fname, uint32_t code) {
    BTreeMap btree = btree_new();

    bool ok = true;
    FILE *bin_fp = fopen(bin_fname, "rb");

    ASSERT(bin_fp);

    DBBusLineHeader header;
    ASSERT(ok = read_header_bus_line(bin_fp, &header));
    ASSERT(ok = btree_load(&btree, index_fname) == BTREE_OK);

    int64_t off = btree_get(&btree, code);
    
    ASSERT(!btree_has_error(&btree));

    if (off < 0) {
        printf("Registro inexistente.\n");
    } else {
        fseek(bin_fp, off, SEEK_SET);

        DBBusLineRegister reg;
        ASSERT(ok = read_bus_line_register(bin_fp, &reg));
        print_bus_line(stdout, &reg, &header);
    }

teardown:

    if (!ok) {
#ifdef DEBUG
        if (btree.error_msg) {
            fprintf(stderr, "Error: %s\n", btree.error_msg);
        } else {
            fprintf(stderr, "Error: unexpected\n");
        }
#else
        printf(ERROR_FOUND);
#endif
    }

    btree_drop(btree);
    fclose(bin_fp);
    return true;
}

#undef ASSERT

typedef struct {
    FILE *bin_fp;
    BTreeMap *btree;
    size_t reg_count;
    size_t removed_reg_count;
} IterArgs;

static CSVResult vehicle_index_row_iterator(CSV *csv, const Vehicle *vehicle, IterArgs *args) {
    size_t offset = ftell(args->bin_fp);

    if (!write_vehicle(vehicle, args->bin_fp)) {
        csv_error(csv, "failed to write vehicle");
        return CSV_ERR_OTHER;
    }

    if (vehicle->prefixo[0] == REMOVED_MARKER) {
        args->removed_reg_count++;
    } else {
        args->reg_count++;

        // Por algum motivo `convertePrefixo` recebe um argumento não `const`, então
        // precisamos desse cast.
        int32_t hash = convertePrefixo((char *)vehicle->prefixo);

        if (!btree_insert(args->btree, hash, offset)) {
            csv_error(csv, "failed to insert vehicle register in index");
            return CSV_ERR_OTHER;
        }
    }
    return CSV_OK;
}

static CSVResult bus_line_index_row_iterator(CSV *csv, const BusLine *bus_line, IterArgs *args) {
    size_t offset = ftell(args->bin_fp);

    if (!write_bus_line(bus_line, args->bin_fp)) {
        csv_error(csv, "failed to write bus line");
        return CSV_ERR_OTHER;
    }

    if (bus_line->codLinha[0] == REMOVED_MARKER) {
        args->removed_reg_count++;
    } else {
        args->reg_count++;

        int32_t codLinha = (int)strtol(bus_line->codLinha, NULL, 10);
        if (!btree_insert(args->btree, codLinha, offset)) {
            csv_error(csv, "failed to insert bus line register in index");
            return CSV_ERR_OTHER;
        }
    }

    return CSV_OK;
}

static bool csv_append_to_bin_and_index(
    const char *bin_fname,
    const char *index_fname,
    CSV *csv,
    CSVIterFunc *iter,
    const char *sep
) {
    FILE *fp = fopen(bin_fname, "r+b");

    if (!fp) {
#ifdef DEBUG
        fprintf(stderr, "Error: Could not open file.\n");
#else
        printf(ERROR_FOUND);
#endif
        return false;
    }

    BTreeMap btree = btree_new();

#ifdef DEBUG
#define ASSERT(expr, ...)                 \
    do {                                  \
        if (!(expr)) {                    \
            fprintf(stderr, __VA_ARGS__); \
            btree_drop(btree);            \
            fclose(fp);                   \
            return false;                 \
        }                                 \
    } while (0)
#else
#define ASSERT(expr, ...)                 \
    do {                                  \
        if (!(expr)) {                    \
            printf(ERROR_FOUND);          \
            btree_drop(btree);            \
            fclose(fp);                   \
            return false;                 \
        }                                 \
    } while (0)
#endif

    ASSERT(btree_load(&btree, index_fname) == BTREE_OK,
           "Error: could not load btree from file '%s'.\n", index_fname);

    DBMeta meta;

    ASSERT(read_meta(fp, &meta),
           "Error: could not read meta header from file '%s'.\n", bin_fname);

    ASSERT(update_header_status('0', fp),
           "Error: could not write status to file '%s'.\n", bin_fname);

    IterArgs args = {
        .bin_fp            = fp,
        .btree             = &btree,
        .reg_count         = 0,
        .removed_reg_count = 0,
    };

    // Vai para o fim do arquivo para adicionar novos registros.
    fseek(fp, 0L, SEEK_END);

    ASSERT(csv_iterate_rows(csv, sep, (CSVIterFunc *)iter, &args) == CSV_OK,
           "Error: %s.\n", csv_get_error(csv));

    meta.status = '1';
    meta.byteProxReg = ftell(fp);
    meta.nroRegRemovidos += args.removed_reg_count;
    meta.nroRegistros += args.reg_count;

    ASSERT(update_header_meta(&meta, fp),
           "Error: could not write meta header to file '%s'.\n", bin_fname);

    fclose(fp);

    return true;

#undef ASSERT
}

bool csv_append_to_bin_and_index_vehicle(const char *bin_fname, const char *index_fname) {
    CSV csv = configure_vehicle_csv();
    csv_use_fp(&csv, stdin);

    bool ok = csv_append_to_bin_and_index(bin_fname, index_fname, &csv, (CSVIterFunc *)vehicle_index_row_iterator, " ");

    csv_drop(csv);
    return ok;
}

bool csv_append_to_bin_and_index_bus_line(const char *bin_fname, const char *index_fname) {
    CSV csv = configure_bus_line_csv();
    csv_use_fp(&csv, stdin);

    bool ok = csv_append_to_bin_and_index(bin_fname, index_fname, &csv, (CSVIterFunc *)bus_line_index_row_iterator, " ");

    csv_drop(csv);
    return ok;
}
