#ifndef _INDEX_H_
#define _INDEX_H_

#include <stdint.h>
#include <stdbool.h>

bool index_vehicle_create(const char *bin_fname, const char *index_fname);
bool index_bus_line_create(const char *bin_fname, const char *index_fname);
bool search_for_vehicle(const char *bin_fname, const char *index_fname, const char prefixo[6]);
bool search_for_bus_line(const char *bin_fname, const char *index_fname, uint32_t code);
bool csv_append_to_bin_and_index_vehicle(const char *bin_fname, const char *index_fname);
bool csv_append_to_bin_and_index_bus_line(const char *bin_fname, const char *index_fname);

#endif
