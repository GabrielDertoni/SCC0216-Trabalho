#ifndef EXTERNAL_H
#define EXTERNAL_H

/*
    Função passada para verificar o arquivo binário escrito
*/
void binarioNaTela(char *nomeArquivoBinario);


/*
*    Use essa função para ler um campo string delimitado entre aspas (").
*    Chame ela na hora que for ler tal campo. Por exemplo:
*
*    A entrada está da seguinte forma:
*        nomeDoCampo "MARIA DA SILVA"
*
*    Para ler isso para as strings já alocadas str1 e str2 do seu programa, você faz:
*        scanf("%s", str1); // Vai salvar nomeDoCampo em str1
*        scan_quote_string(str2); // Vai salvar MARIA DA SILVA em str2 (sem as aspas)
*
*/
void scan_quote_string(char *str);

/*  
    Converte o prefixo do veículo para int

    OBS1:   retorna -1 se o primeiro caracter é '*'

    OBS2:   retorna LIXO se a string contém algo diferente de números e letras 
            maiúsculas (com excessão do caso da OBS1)

    COMO FUNCIONA:

        Para converter os prefixos para int e garantir que prefixos diferentes 
        deem números diferentes interpretamos eles como números em base 36

        Um número N com 5 digitos dI em base 36 é escrito como:

            N = d0 * 36^0 + d1 * 36^1 + d2 * 36^2 + d3 * 36^3 + d4 * 36^4

        Nota-se que estamos escrevendo do digito menos significativo para o 
        mais significativo

        Como o nosso prefixo têm 5 bytes e eles podem assumir 36 valores
        cada, podemos interpretar cada char dele como um digito em base 36, 
        prefixos diferentes formaram números diferentes em base 36 e portanto 
        números diferentes quando convertemos para um int normal
*/
int convertePrefixo(char* str);

#endif
