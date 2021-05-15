/*
 * Módulo especializado em leitura dos arquivos .csv especificados no projeto.
 * Esse módulo, diferentemente do módulo `csv`, não é genérico mas
 * especializado. Sua função é abstrair o processo de descrever como o .csv deve
 * ser lido.
 */


#ifndef PARSING_H
#define PARSING_H

#include <csv.h>

// Configura um tipo CSV que possa ler veiculo.csv
CSV configure_vehicle_csv();

// Configura um tipo CSV que possa ler linha.csv
CSV configure_bus_line_csv();

typedef struct {
    FILE *fp;
    size_t reg_count;
    size_t removed_reg_count;
} VehicleIterArgs;

CSVResult vehicle_row_iterator(CSV *csv, const Vehicle *vehicle, VehicleIterArgs *args);

#endif
