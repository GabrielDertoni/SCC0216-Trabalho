/**
 * Módulo de leitura e escrita nos arquivos binários.
 *
 * Esse módulo possui uma coleção de funções que leem e escrevem em arquivos
 * binários com registros de veículos ou de linhas de ônibus. Cada função
 * retorna um `bool` que indica se a operação foi ou não um sucesso.
 * 
 * Além disso há uma convensão no prefixo dos nomes:
 *  > update - funções que podem mover o ponteiro de arquivo e escrevem nele.
 *  > write  - funções que escrevem num arquivo sem mover o ponteiro.
 *  > read   - funções que leem de um arquivo sem mover o ponteiro
 *
 */

#ifndef BIN_H
#define BIN_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

// Atualiza o byte indicador de status do aquivo.
// NOTE: Se necessário, reposiciona o ponteiro do arquivo.
bool update_header_status(char new_val, FILE *fp);

// Atualiza o offset para próximo registro do arquivo.
// NOTE: Se necessário, reposiciona o ponteiro do arquivo.
bool update_header_byte_next_reg(uint64_t byte_next_reg, FILE *fp);

// Atualiza o número de registros no arquivo.
// NOTE: Se necessário, reposiciona o ponteiro do arquivo.
bool update_header_n_reg(uint32_t n_reg, FILE *fp);

// Atualiza o número de registros removidos no arquivo.
// NOTE: Se necessário, reposiciona o ponteiro do arquivo.
bool update_header_n_reg_removed(uint32_t n_reg_removed, FILE *fp);

// Escreve o cabeçalho de metadados no arquivo.
// NOTE: Não reposiciona o ponteiro de arquivo.
bool write_header_meta(DBMeta meta, FILE *fp);

// Escreve o cabeçalho do arquivo binário de veículo.
// NOTE: Não reposiciona o ponteiro do arquivo.
bool write_vehicles_header(DBVehicleHeader header, FILE *fp);

// Escreve um registro de veículo no arquivo.
// NOTE: Não reposiciona o ponteiro do arquivo.
bool write_vehicle(const Vehicle *vehicle, FILE *fp);

// Escreve o cabeçalho de linha de ônibus no arquivo.
// NOTE: Não reposiciona o ponteiro do arquivo.
bool write_bus_lines_header(DBBusLineHeader header, FILE *fp);

// Escreve um registro de linha de ônibus no arquivo.
// NOTE: Não reposiciona o ponteiro do arquivo.
bool write_bus_line(const BusLine *line, FILE *fp);

#endif
