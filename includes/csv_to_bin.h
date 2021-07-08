#ifndef _CSV_TO_BIN_H
#define _CSV_TO_BIN_H

#include <stdbool.h>

// Converte um csv de veículos para um binário contendo os registros. Retorna
// `true` caso a operação tenha sido bem sucedida e `false` caso contrário.
bool vehicle_csv_to_bin(const char *csv_fname, const char *bin_fname);

// Converte um csv de linhas de ônibus para um binário contendo os registros.
// Retorna `true` caso a operação tenha sido bem sucedida e `false` caso
// contrário.
bool bus_line_csv_to_bin(const char *csv_fname, const char *bin_fname);

// Lê da entrada padrão vários registros de veículos e escreve esses registros
// no arquivo `bin_fname`.
bool vehicle_append_to_bin_from_stdin(const char *bin_fname);

// Lê da entrada padrão vários registros de linhas de ônibus e escreve esses
// registros no arquivo `bin_fname`.
bool bus_line_append_to_bin_from_stdin(const char *bin_fname);

#endif
