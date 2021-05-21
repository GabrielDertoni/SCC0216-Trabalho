#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <common.h>
#include <parsing.h>
#include <utils.h>
#include <csv.h>
#include <bin.h>
#include <external.h>

typedef enum {
	OP_CREATE_TABLE_VEHICLE       = 1,
	OP_CREATE_TABLE_BUS_LINE      = 2,
	OP_SELECT_FROM_VEHICLE        = 3,
	OP_SELECT_FROM_BUS_LINE       = 4,
	OP_SELECT_FROM_VEHICLE_WHERE  = 5,
	OP_SELECT_FROM_BUS_LINE_WHERE = 6,
	OP_INSERT_INTO_VEHICLE        = 7,
	OP_INSERT_INTO_BUS_LINE       = 8,
} Op;

int main(void){
    int operacao;
    scanf("%d", &operacao);
    fgetc(stdin); // Consome o espaço (32) logo após o inteiro
    char *file_name = read_word(stdin), *input1 = NULL, *input2 = NULL;

    switch (operacao) {
        case OP_CREATE_TABLE_VEHICLE:
            input1 = read_word(stdin);
            if (CREATE_TABLE_FILE(file_name, input1))
                binarioNaTela(input1);
            break;

        case OP_CREATE_TABLE_BUS_LINE:
            input1 = read_word(stdin);
            if (CREATE_TABLE_LINE(file_name, input1))
				binarioNaTela(input1);
            break; 

        case OP_SELECT_FROM_VEHICLE:
            SELECT_FROM_WHERE_FILE(file_name, NULL, NULL);
            break;

        case OP_SELECT_FROM_BUS_LINE:
            SELECT_FROM_WHERE_LINE(file_name, NULL, NULL);
            break;

        case OP_SELECT_FROM_VEHICLE_WHERE:
            input1 = read_word(stdin);
            input2 = read_word(stdin);
            SELECT_FROM_WHERE_FILE(file_name, input1, input2);
            break;

        case OP_SELECT_FROM_BUS_LINE_WHERE:
            input1 = read_word(stdin);
            input2 = read_word(stdin);
            SELECT_FROM_WHERE_LINE(file_name, input1, input2);
            break;

        case OP_INSERT_INTO_VEHICLE:
            input1 = read_word(stdin);
            if(INSERT_INTO_FILE(file_name))
                binarioNaTela(file_name);
            break;

        case OP_INSERT_INTO_BUS_LINE:
            input1 = read_word(stdin);
            if(INSERT_INTO_LINE(file_name))
                binarioNaTela(file_name);
            break;
    }

    if (file_name != NULL)
        free(file_name);

    if (input1 != NULL)
        free(input1);

    if (input2 != NULL)
        free(input2);

    return 0;
}
