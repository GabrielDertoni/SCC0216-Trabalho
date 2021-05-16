#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <common.h>
#include <parsing.h>
#include <csv.h>
#include <bin.h>
#include <external.h>

int main(int argc, char *argv[]) {
    vehicle_csv_to_bin("data/veiculo.csv", "veiculo.bin");
    bus_line_csv_to_bin("data/linha.csv", "linha.bin");

    printf("veiculo.bin: ");
    binarioNaTela("veiculo.bin");

    printf("linha.bin: ");
    binarioNaTela("linha.bin");
    return 0;
}
