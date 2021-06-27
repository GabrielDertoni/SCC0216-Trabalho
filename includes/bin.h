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

#include <common.h>

// Atualiza o byte indicador de status do aquivo.
bool update_header_status(char new_val, FILE *fp);

// Atualiza o cabeçalho meta do arquivo.
bool update_header_meta(DBMeta meta, FILE *fp);

// Escreve o cabeçalho de metadados no arquivo.
bool write_header_meta(DBMeta meta, FILE *fp);

// Escreve o cabeçalho do arquivo binário de veículo.
bool write_vehicles_header(DBVehicleHeader header, FILE *fp);

// Escreve um registro de veículo no arquivo.
bool write_vehicle(const Vehicle *vehicle, FILE *fp);

// Escreve o cabeçalho de linha de ônibus no arquivo.
bool write_bus_lines_header(DBBusLineHeader header, FILE *fp);

// Escreve um registro de linha de ônibus no arquivo.
bool write_bus_line(const BusLine *line, FILE *fp);

// Lê os meta dados de um arquivo fp e salva os valores nos campos correspondentes de meta.
bool read_meta(FILE *fp, DBMeta *meta);

// Lê o cabeçalho de um arquivo binário que contém os registros das linhas de ônibus
bool read_header_bus_line(FILE *fp, DBBusLineHeader *header);

// Lê o cabeçalho de um arquivo binário que contém os registros de veículo
bool read_header_vehicle(FILE *fp, DBVehicleHeader *header);

/*
 * Lê os registros de um arquivo binário de veículos
 * @param fp - ponteiro do arquivo binário
 * @param reg - ponteiro de DBVehicleRegister
 * @return 'true' se for lido com sucesso 'false' se houver algum erro
*/ 
bool read_vehicle_register(FILE *fp, DBVehicleRegister *reg);

/*
 * Lê os registros de um arquivo binário de linhas de ônibus
 * @param fp - ponteiro do arquivo binário
 * @param reg - ponteiro de DBBusLineRegister
 * @return 'true' se for lido com sucesso 'false' se houver algum erro
*/
bool read_bus_line_register(FILE *fp, DBBusLineRegister *reg);

/* 
  * Seleciona os dados de uma tabela que contém os arquivos veículos
  * @param from_file - caminho do arquivo binário que contém os veículos
  * @param where_field - nome do campo a ser buscado na tabela -> NULL exibe todos os valores
  * @param equals_to - valor do campo a ser buscado na tabela -> NULL exibe todos os valores
*/
bool select_from_vehicle_where(const char *from_file, const char *where_field, const char *equals_to);

/* 
  * Seleciona os dados de uma tabela que contém os arquivos das linhas de ônibus
  * @param from_file - caminho do arquivo binário que contém as linhas de ônibus
  * @param where_field - nome do campo a ser buscado na tabela -> NULL exibe todos os valores
  * @param equals_to - valor do campo a ser buscado na tabela -> NULL exibe todos os valores
*/
bool select_from_bus_line_where(const char *from_file, const char *where_campo, const char *where_valor);

#endif
