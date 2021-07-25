#ifndef _SORT_H_
#define _SORT_H_

#include <stdbool.h>

/*
* Lê os dados de um arquivo binário de veículos em RAM, ordena esses dados e os escreve em um novo arquivo binário.
* @param bin_fname - caminho para o arquivo binário de veículos a ser lido
* @param ordered_bin_fname - caminho para o arquivo binário de veículos ordenado a ser escrito
* @returns - um valor booleano = true deu tudo certo, false algum erro ocorreu (uma mensagem de erro será exibida)
*/
bool sort_vehicle_bin_file(const char *bin_fname, const char *ordered_bin_fname);

/*
* Lê os dados de um arquivo binário de linhas de ônibus em RAM, ordena esses dados e os escreve em um novo arquivo binário.
* @param bin_fname - caminho para o arquivo binário de linhas de ônibus a ser lido
* @param ordered_bin_fname - caminho para o arquivo binário de linhas de ônibus ordenado a ser escrito
* @returns - um valor booleano = true deu tudo certo, false algum erro ocorreu (uma mensagem de erro será exibida)
*/
bool sort_bus_line_bin_file(const char *bin_fname, const char *ordered_bin_fname);

#endif
