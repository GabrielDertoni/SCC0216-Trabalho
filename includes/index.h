/**
 * Esse módulo é responsável por todas as operações que envolvem o index de
 * BTree. Ou seja, ele se comunica com o módulo de btree e também com o módulo
 * binário e csv para poder ler os registros da entrada padrão.
 */


#ifndef _INDEX_H_
#define _INDEX_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * Cria um arquivo de indice arvore-B para o arquivo de dados veiculo
 * @params bin_fname - nome do arquivo binario veiculos
 * @params index_fname - nome do arquivo binario de indice arvore-B
 * @returns um valor booleano - true se for criado, false se der algum erro
 */
bool index_vehicle_create(const char *bin_fname, const char *index_fname);

/**
 * Cria um arquivo de indice arvore-B para o arquivo de dados linhas de onibus
 * @params bin_fname - nome do arquivo binario linhas de onibus
 * @params index_fname - nome do arquivo binario de indice arvore-B
 * @returns um valor booleano - true se for criado, false se der algum erro
 */
bool index_bus_line_create(const char *bin_fname, const char *index_fname);

/**
 * Recupera os regstros buscados de um determinado arquivo de dados veiculo usando o indice arvore-B
 * @params bin_fname - nome do arquivo binario veiculos
 * @params index_fname - nome do arquivo binario de indices arvore-B
 * @params prefixo[6] - valor do campo prefixo em que sera feita a busca
 * @returns um valor booleano - true se a busca for feita com sucesso, false se ocorrer algum erro
 */
bool search_for_vehicle(const char *bin_fname, const char *index_fname, const char prefixo[6]);

/**
 * Recupera os regstros buscados de um determinado arquivo de dados lnhas de onibus usando o indice arvore-B
 * @params bin_fname - nome do arquivo binario linhas de onibus
 * @params index_fname - nome do arquivo binario de indices arvore-B
 * @params prefixo[6] - valor do campo prefixo em que sera feita a busca
 * @returns um valor booleano - true se a busca for feita com sucesso, false se ocorrer algum erro
 */
bool search_for_bus_line(const char *bin_fname, const char *index_fname, uint32_t code);


/**
 * Insere cada registro em um arquivo binário de dados veículo e a chave de busca correspondente a essa inserção inserida no indice arvore-B
 *
 * @params bin_fname - string que corresponde ao nome do arquivo binario de veiculos
 * @params index_fname - string que corresponde ao nome do arquivo binario de indices arvore-B
 * @returns um valor booleano - true se a insercao ocorrer e false se nao ocorrer
 */
bool csv_append_to_bin_and_index_vehicle(const char *bin_fname, const char *index_fname);

/**
 * Insere cada registro em um arquivo binário de dados linha de onibus e a chave de busca correspondente a essa inserção inserida no indice arvore-B
 *
 * @params bin_fname - string que corresponde ao nome do arquivo binario de linhas de onibus
 * @params index_fname - string que corresponde ao nome do arquivo binario de indices arvore-B
 * @returns um valor booleano - true se a insercao ocorrer e false se nao ocorrer
 */
bool csv_append_to_bin_and_index_bus_line(const char *bin_fname, const char *index_fname);

#endif
