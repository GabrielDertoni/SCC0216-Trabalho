#ifndef _BTREE_H_
#define _BTREE_H_

#include <stdio.h>
#include <stdint.h>

typedef struct {
    FILE *fp;
    char *error_msg;
    int32_t rrn_root;
    uint32_t next_rrn;
} BTreeMap;

typedef enum {
    BTREE_OK,
    BTREE_FAIL,
} BTreeResult;

BTreeResult btree_insert(BTreeMap *btree, int32_t key, uint64_t value);
BTreeMap btree_new();
void btree_drop(BTreeMap btree);
BTreeResult btree_load(BTreeMap *btree, const char *fname);
BTreeResult btree_create(BTreeMap *btree, const char *fname);

void btree_print(BTreeMap *btree);

#endif
