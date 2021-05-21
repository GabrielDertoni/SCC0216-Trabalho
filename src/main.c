#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <common.h>
#include <parsing.h>
#include <csv.h>
#include <bin.h>
#include <external.h>

// #include<sql.h>

#define QUOTE 34
#define SPACE 32
#define BREAK_LINE 10
#define CARRIAGE_RETURN 13
#define BUFFER 4096
#define and &&

char *LerPalavraSemCitacao(FILE*);

int main(void){
    int operacao;
    scanf("%d", &operacao);
    fgetc(stdin); // Consome o espaço (32) logo após o inteiro
    char *file_name = LerPalavraSemCitacao(stdin), *input1 = NULL, *input2 = NULL;
    switch(operacao){
        case 1:
            input1 = LerPalavraSemCitacao(stdin);
            if(CREATE_TABLE_FILE(file_name, input1))
                binarioNaTela(input1);
            break;
        case 2:
            input1 = LerPalavraSemCitacao(stdin);
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
            input1 = LerPalavraSemCitacao(stdin);
            input2 = LerPalavraSemCitacao(stdin);
            SELECT_FROM_WHERE_FILE(file_name, input1, input2);
            break;
        case 6:
            input1 = LerPalavraSemCitacao(stdin);
            input2 = LerPalavraSemCitacao(stdin);
            SELECT_FROM_WHERE_LINE(file_name, input1, input2);
            break;
        case 7:
            input1 = LerPalavraSemCitacao(stdin);
            if(INSERT_INTO_FILE(file_name))
                binarioNaTela(file_name);
            break;
        case 8:
            input1 = LerPalavraSemCitacao(stdin);
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

char *LerPalavraSemCitacao(FILE *in){
    char *string = NULL;
    long start, end;
    int character;
    start = end = ftell(in);
    do{
        if(end-start % BUFFER == 0)
            string = realloc(string, (end-start / BUFFER + 1) * BUFFER);
        character = fgetc(in);
        if(character != CARRIAGE_RETURN and character != QUOTE){
            string[end-start] = character;
            end++;
        }
    }while(character != SPACE and character != BREAK_LINE and character != EOF);
    string[end-start-1] = 0;
    string = realloc(string, end-start);
    return string;
}
