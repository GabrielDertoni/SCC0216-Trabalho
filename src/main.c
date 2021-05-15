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
    vehicle_csv_to_bin("data/veiculo.csv", "veiculo.bin");
    bus_line_csv_to_bin("data/linha.csv", "linha.bin");
    return 0;
}
