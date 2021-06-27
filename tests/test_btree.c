#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <btree.h>

#define ASSERT(btree, expr)                                  \
    do {                                                     \
        if (!(expr)) {                                       \
            fprintf(stderr, "Error: %s\n", btree.error_msg); \
            goto teardown;                                   \
        }                                                    \
    } while (0);

int main() {
    system("mkdir -p tmp");

    bool ok;

    BTreeMap btree = btree_new();

    ASSERT(btree, ok = btree_create(&btree, "tmp/mybtree.bin") == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'a', 0x1) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'b', 0x2) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'c', 0x3) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'd', 0x4) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'e', 0x5) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'f', 0x6) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'g', 0x7) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'h', 0x8) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'i', 0x9) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'j', 0xa) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'k', 0xb) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'Z', 0xc) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'X', 0xd) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'Y', 0xd) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'W', 0xd) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'V', 0xd) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

    ASSERT(btree, ok = btree_insert(&btree, 'P', 0xd) == BTREE_OK);
    btree_print(&btree);
    printf("\n");

teardown:
    btree_drop(btree);

    if (!ok) return 1;
    return 0;
}
