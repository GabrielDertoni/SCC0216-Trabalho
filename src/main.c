#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <common.h>
#include <parsing.h>
#include <csv.h>
#include <bin.h>
#include <external.h>

#include <errno.h>

// #include<sql.h>

#define QUOTE 34
#define SPACE 32
#define BREAK_LINE 10
#define CARRIAGE_RETURN 13
#define BUFFER 4096
#define and &&

char *read_word(FILE *in);

int main(void){
    int operacao;
    scanf("%d", &operacao);
    fgetc(stdin); // Consome o espaço (32) logo após o inteiro
    char *file_name = read_word(stdin), *input1 = NULL, *input2 = NULL;
    switch(operacao){
        case 1:
            input1 = read_word(stdin);
            if(CREATE_TABLE_FILE(file_name, input1))
                binarioNaTela(input1);
            break;
        case 2:
            input1 = read_word(stdin);
            CREATE_TABLE_LINE(file_name, input1);
            binarioNaTela(input1);
            break; 
        case 3:
            SELECT_FROM_WHERE_FILE(file_name, NULL, NULL);
            break;
        case 4:
            SELECT_FROM_WHERE_LINE(file_name, NULL, NULL);
            break;
        case 5:
            input1 = read_word(stdin);
            input2 = read_word(stdin);
            SELECT_FROM_WHERE_FILE(file_name, input1, input2);
            break;
        case 6:
            input1 = read_word(stdin);
            input2 = read_word(stdin);
            SELECT_FROM_WHERE_LINE(file_name, input1, input2);
            break;
        case 7:
            input1 = read_word(stdin);
            if(INSERT_INTO_FILE(file_name))
                binarioNaTela(file_name);
            break;
        case 8:
            input1 = read_word(stdin);
            if(INSERT_INTO_LINE(file_name))
                binarioNaTela(file_name);
            break;
    }
    if(file_name != NULL)
        free(file_name);
    if(input1 != NULL)
        free(input1);
    if(input2 != NULL)
        free(input2);

    return 0;
}

char *read_word(FILE *in) {
    char *string = NULL;
	size_t len = 0;
    int character;

    do {
        if(len % BUFFER == 0)
            string = realloc(string, (len / BUFFER + 1) * BUFFER + 1);

        character = fgetc(in);

        if(character != CARRIAGE_RETURN and character != QUOTE)
            string[len++] = character;

    } while (character != SPACE and character != BREAK_LINE and character != EOF);

    string[len-1] = '\0';
    string = realloc(string, len);
    return string;
}
