#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <common.h>
#include <csv.h>
#include <bin.h>

CSV configure_vehicle_csv() {
    CSV csv = csv_new(sizeof(Vehicle), 6);
    csv_set_column(&csv, 0, csv_column_default(char  , Vehicle, prefixo          , "\0@@@@"));
    csv_set_column(&csv, 1, csv_column_default(char  , Vehicle, data             , "\0@@@@@@@@@"));
    csv_set_column(&csv, 2, csv_column_default(i32   , Vehicle, quantidadeLugares, -1));
    csv_set_column(&csv, 3, csv_column_default(i32   , Vehicle, codLinha         , -1));
    csv_set_column(&csv, 4, csv_column(string, Vehicle, modelo));
    csv_set_column(&csv, 5, csv_column(string, Vehicle, categoria));
    return csv;
}

CSV configure_bus_line_csv() {
    CSV csv = csv_new(sizeof(BusLine), 4);
    csv_set_column(&csv, 0, csv_column_default(i32 , BusLine, codLinha    , -1));
    csv_set_column(&csv, 1, csv_column_default(char, BusLine, aceitaCartao, '\0'));
    csv_set_column(&csv, 2, csv_column_default(i32 , BusLine, nomeLinha   , -1));
    csv_set_column(&csv, 3, csv_column_default(i32 , BusLine, corLinha    , -1));
    return csv;
}

int main(int argc, char *argv[]) {
    CSV vehicles_csv = configure_vehicle_csv();

    ParserResult res = csv_parse_file(&vehicles_csv, "data/veiculos.csv", ",");

    if (res == PARSER_FAIL) {
        fprintf(stderr, "Error: %s.\n", vehicles_csv.error_msg);
        csv_drop(vehicles_csv);
        return 1;
    }

    Vehicle *vehicles = csv_get_values(&vehicles_csv, Vehicle);

    printf("Vehicle 1:\n");
    printf("prefixo: %.*s\n", 5, vehicles[0].prefixo);
    printf("data: %.*s\n", 10, vehicles[0].data);
    printf("quantidadeLugares: %d\n", vehicles[0].quantidadeLugares);
    printf("codLinha: %d\n", vehicles[0].codLinha);
    printf("modelo: %s\n", vehicles[0].modelo);
    printf("categoria: %s\n", vehicles[0].categoria);
    printf("\n");

    uint32_t n_vehicles = csv_row_count(&vehicles_csv);

    write_vehicles(vehicles, n_vehicles, "out.bin");

    csv_drop(vehicles_csv);

    return 0;
}
