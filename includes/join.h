#ifndef _JOIN_H_
#define _JOIN_H_

#include <stdbool.h>

/*
* Exibe os resultados que satisfazem a busca de codLinha no arquivo binário de veículos e no arquivo binário de linhsa de ônibus 
* @param vehiclebin_fname - caminho para o arquivo binário de veículos
* @param buslinebin_fname - caminho para o arquivo binário de linhas de ônibus
* @returns - um valor booleano = true se a leitura dos arquivos der certo e retornar algum resultado, false se a leitura dos arquivos der errado ou não retornar nenhum resultado da busca
*/
bool join_vehicle_and_bus_line(const char *vehiclebin_fname, const char *buslinebin_fname);

/*
* Exibe os resultados que satisfazem a busca de codLinha no arquivo binário de veículos e no arquivo binário de linhsa de ônibus 
* @param vehiclebin_fname - caminho para o arquivo binário de veículos
* @param buslinebin_fname - caminho para o arquivo binário de linhas de ônibus
* @param index_btree_fname - caminho para o arquivo binário de índices árvore-B
* @returns - um valor booleano = true se a leitura dos arquivos der certo e retornar algum resultado, false se a leitura dos arquivos der errado ou não retornar nenhum resultado da busca
*/
bool join_vehicle_and_bus_line_using_btree(const char *vehiclebin_fname, const char *buslinebin_fname, const char *index_btree_fname);

/**
 * Ordena os dois arquivos fornecidos gerando novos arquivos ordenados com o
 * sufixo "_ordenado". Em seguida abre esses arquivos ordenados e itera sobre os
 * registros imprimindo os registros onde veiculo.codLinha == linha.codLinha
 * usando um merge.
 *
 * @param vehicle_bin_fname - nome do arquivo binário com os registros de veículo.
 * @param busline_bin_fname - nome do arquivo binário com os registros de linha.
 * @returns - um valor booleano = true deu tudo certo, false algum erro ocorreu
 *            (uma mensagem de erro será exibida).
 */
bool merge_sorted(const char *vehicle_bin_fname, const char *busline_bin_fname);

#endif
