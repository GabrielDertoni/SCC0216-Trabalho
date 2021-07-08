/*
 * Módulo especializado em leitura dos arquivos .csv especificados no projeto.
 * Esse módulo, diferentemente do módulo `csv`, não é genérico mas
 * especializado. Sua função é abstrair o processo de descrever como o .csv deve
 * ser lido.
 */

#ifndef PARSING_H
#define PARSING_H

#include <common.h>
#include <csv.h>

// Configura um tipo `CSV` que possa ler "veiculo.csv"
CSV configure_vehicle_csv();

// Configura um tipo `CSV` que possa ler "linha.csv"
CSV configure_bus_line_csv();

#endif
