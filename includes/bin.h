#ifndef BIN_H
#define BIN_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

bool update_header_status(bool new_val, FILE *fp);
bool update_header_byte_next_reg(uint64_t byte_next_reg, FILE *fp);
bool update_header_n_reg(uint32_t n_reg, FILE *fp);
bool update_header_n_reg_removed(uint32_t n_reg_removed, FILE *fp);
bool write_header_meta(DBMeta meta, FILE *fp);
bool write_vehicles_header(uint32_t nroRegistros, FILE *fp);
bool write_vehicle(Vehicle vehicle, FILE *fp);
bool write_vehicles(Vehicle *vehicles, uint32_t n_vehicles, const char *fname);
bool write_bus_lines_header(uint32_t nroRegistros, FILE *fp);
bool write_bus_line(BusLine line, FILE *fp);
bool write_bus_lines(BusLine *lines, uint32_t n_lines, const char *fname);

#endif
