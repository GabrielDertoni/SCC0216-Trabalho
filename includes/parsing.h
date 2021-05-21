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

// Tipo que contém os argumentos adicionais para as funções iteradoras.
typedef struct {
    FILE *fp;
    size_t reg_count;
    size_t removed_reg_count;
} IterArgs;

// Função que será executada para cada linha do `csv` para veículos.
CSVResult vehicle_row_iterator(CSV *csv, const Vehicle *vehicle, IterArgs *args);

// Função que será executada para cada linha do `csv` para linhas de ônibus.
CSVResult bus_line_row_iterator(CSV *csv, const BusLine *bus_line, IterArgs *args);

// Converte um csv de veículos para um binário contendo os registros. Retorna
// `true` caso a operação tenha sido bem sucedida e `false` caso contrário.
// bool vehicle_csv_to_bin(const char *csv_fname, const char *bin_fname);
bool CREATE_TABLE_FILE(const char *csv_fname, const char *bin_fname);

// Converte um csv de linhas de ônibus para um binário contendo os registros.
// Retorna `true` caso a operação tenha sido bem sucedida e `false` caso
// contrário.
// bool bus_line_csv_to_bin(const char *csv_fname, const char *bin_fname);
bool CREATE_TABLE_LINE(const char *csv_fname, const char *bin_fname);


// Lê da entrada padrão vários registros de veículos e escreve esses registros
// no arquivo `bin_fname`.
bool INSERT_INTO_FILE(const char *bin_fname);

// Lê da entrada padrão vários registros de linhas de ônibus e escreve esses
// registros no arquivo `bin_fname`.
bool INSERT_INTO_LINE(const char *bin_fname);

#endif
