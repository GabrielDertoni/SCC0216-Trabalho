#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <common.h>
#include <parsing.h>
#include <csv.h>
#include <bin.h>
#include <external.h>

#define CSV_ASSERT(csv, expr) \
    if (!(expr)) \
        print_error_exit(csv) \

void print_error_exit(CSV csv) {
    fprintf(stderr, "Error: %s.\n", csv.error_msg);

    csv_drop(csv);
    exit(1);
}

int main(int argc, char *argv[]) {
    CSV vehicles_csv = configure_vehicle_csv();

    FILE *fp = fopen("veiculo.bin", "w");

    if (!fp) {
        fprintf(stderr, "Error: Could not open file.\n");
        return 1;
    }

    CSV_ASSERT(vehicles_csv, !CSV_IS_ERROR(csv_open(&vehicles_csv, "data/veiculo.csv")));
    CSV_ASSERT(vehicles_csv, !CSV_IS_ERROR(csv_parse_header(&vehicles_csv, ",")));

    char status = '0';
    fwrite(&status, sizeof(char), 1, fp);
    fseek(fp, VEHICLE_HEADER_SIZE + 1, SEEK_SET);

    VehicleIterArgs args = {
        .fp                = fp,
        .reg_count         = 0,
        .removed_reg_count = 0,
    };

    CSV_ASSERT(vehicles_csv,
        !CSV_IS_ERROR(
            csv_iterate_rows(
                &vehicles_csv, ",",
                (IterFunc *)vehicle_row_iterator, &args
            )
        )
    );

    DBMeta meta = {
        .status          = '1',
        .byteProxReg     = ftell(fp),
        .nroRegistros    = args.reg_count,
        .nroRegRemovidos = args.removed_reg_count,
    };

    DBVehicleHeader header = { .meta = meta };

    memcpy(&header.descrevePrefixo  , vehicles_csv.columns[0].name, sizeof(header.descrevePrefixo));
    memcpy(&header.descreveData     , vehicles_csv.columns[1].name, sizeof(header.descreveData));
    memcpy(&header.descreveLugares  , vehicles_csv.columns[2].name, sizeof(header.descreveLugares));
    memcpy(&header.descreveLinhas   , vehicles_csv.columns[3].name, sizeof(header.descreveLinhas));
    memcpy(&header.descreveModelo   , vehicles_csv.columns[4].name, sizeof(header.descreveModelo));
    memcpy(&header.descreveCategoria, vehicles_csv.columns[5].name, sizeof(header.descreveCategoria));

    fseek(fp, 0L, SEEK_SET);
    write_vehicles_header(header, fp);

    fclose(fp);
    csv_drop(vehicles_csv);

    binarioNaTela("veiculo.bin");

    return 0;
}
