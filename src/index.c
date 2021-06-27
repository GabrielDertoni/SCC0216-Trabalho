#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <external.h>
#include <index.h>
#include <btree.h>
#include <bin.h>
#include <common.h>

#define ASSERT(expr) \
    if (!(expr)) \
        goto teardown

bool index_vehicle_create(const char *bin_fname, const char *index_fname) {
    BTreeMap btree = btree_new();

    bool ok = true;
    FILE *bin_fp = fopen(bin_fname, "rb");

    if (!bin_fp) {
        printf(ERROR_FOUND);
        return false;
    }

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
            uint32_t hash = convertePrefixo(reg.prefixo);
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

    if (!bin_fp) {
        printf(ERROR_FOUND);
        return false;
    }

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
