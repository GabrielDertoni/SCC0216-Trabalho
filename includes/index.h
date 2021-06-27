#ifndef _INDEX_H_
#define _INDEX_H_

#include <stdbool.h>

bool index_vehicle_create(const char *bin_fname, const char *index_fname);
bool index_bus_line_create(const char *bin_fname, const char *index_fname);

#endif
