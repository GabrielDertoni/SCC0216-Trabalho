#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <common.h>
#include <parsing.h>
#include <utils.h>
#include <csv.h>
#include <index.h>
#include <btree.h>
#include <bin.h>
#include <external.h>

typedef enum {
    OP_CREATE_TABLE_VEHICLE       =  1,
    OP_CREATE_TABLE_BUS_LINE      =  2,
    OP_SELECT_FROM_VEHICLE        =  3,
    OP_SELECT_FROM_BUS_LINE       =  4,
    OP_SELECT_FROM_VEHICLE_WHERE  =  5,
    OP_SELECT_FROM_BUS_LINE_WHERE =  6,
    OP_INSERT_INTO_VEHICLE        =  7,
    OP_INSERT_INTO_BUS_LINE       =  8,
    OP_CREATE_INDEX_VEHICLE       =  9,
    OP_CREATE_INDEX_BUS_LINE      = 10,
    OP_SEARCH_FOR_VEHICLE         = 11,
    OP_SEARCH_FOR_BUS_LINE        = 12,
} Op;

void test(const char *fname) {
    BTreeMap btree = btree_new();
    btree_load(&btree, fname);

    btree_print(&btree);

    btree_drop(btree);
}

int main(void){
    int operacao;
    scanf("%d ", &operacao);
    char *file_name = read_word(stdin), *input1 = NULL, *input2 = NULL;

    switch (operacao) {
        case OP_CREATE_TABLE_VEHICLE:
            input1 = read_word(stdin);
            if (vehicle_csv_to_bin(file_name, input1))
                binarioNaTela(input1);
            break;

        case OP_CREATE_TABLE_BUS_LINE:
            input1 = read_word(stdin);
            if (bus_line_csv_to_bin(file_name, input1))
                binarioNaTela(input1);
            break; 

        case OP_SELECT_FROM_VEHICLE:
            select_from_vehicle_where(file_name, NULL, NULL);
            break;

        case OP_SELECT_FROM_BUS_LINE:
            select_from_bus_line_where(file_name, NULL, NULL);
            break;

        case OP_SELECT_FROM_VEHICLE_WHERE:
            input1 = read_word(stdin);
            input2 = read_word(stdin);
            select_from_vehicle_where(file_name, input1, input2);
            break;

        case OP_SELECT_FROM_BUS_LINE_WHERE:
            input1 = read_word(stdin);
            input2 = read_word(stdin);
            select_from_bus_line_where(file_name, input1, input2);
            break;

        case OP_INSERT_INTO_VEHICLE:
            input1 = read_word(stdin);
            if(vehicle_append_to_bin_from_stdin(file_name))
                binarioNaTela(file_name);
            break;

        case OP_INSERT_INTO_BUS_LINE:
            input1 = read_word(stdin);
            if(bus_line_append_to_bin_from_stdin(file_name))
                binarioNaTela(file_name);
            break;

        case OP_CREATE_INDEX_VEHICLE:
            input1 = read_word(stdin);
            if (index_vehicle_create(file_name, input1))
                binarioNaTela(input1);
            break;

        case OP_CREATE_INDEX_BUS_LINE:
            input1 = read_word(stdin);
            if (index_bus_line_create(file_name, input1))
                binarioNaTela(input1);
            break;

        case OP_SEARCH_FOR_VEHICLE: {
            input1 = read_word(stdin);
            input2 = read_word(stdin); // Garantido de ser "prefixo"

            char prefixo[6];
            scanf("\"%[^\"]s\"", prefixo);
            search_for_vehicle(file_name, input1, prefixo);
            break;
        }

        case OP_SEARCH_FOR_BUS_LINE: {
            input1 = read_word(stdin);
            input2 = read_word(stdin); // Garantido de ser "codLinha"

            uint32_t code;
            scanf(" %d", &code);
            search_for_bus_line(file_name, input1, code);
            break;
        }
    }

    if (file_name != NULL)
        free(file_name);

    if (input1 != NULL)
        free(input1);

    if (input2 != NULL)
        free(input2);

    return 0;
}
