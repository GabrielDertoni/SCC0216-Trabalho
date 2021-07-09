/**
 * Módulo da BTreeMap.
 *
 * Esse módulo consiste da implementação de uma BTree em disco. Ele não envolve
 * qualquer alocação dinâmica para operar normalmente e talvez apenas aloque
 * memória para mensagens de erro. Além disso, a BTree em disco é atualizada e
 * verificada somente quando necessário.
 */


#ifndef _BTREE_H_
#define _BTREE_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

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

/**
 * Cria um novo `BtreeMap`. Não envolve alocação ou abertura de arquivos.
 *
 * @return uma BTree ainda sem arquivo vinculado.
 */
BTreeMap btree_new();

/**
 * Libera o `BTreeMap` inclusive fechando algum arquivo vinculado.
 *
 * @param btree - a btree a ser liberada.
 */
void btree_drop(BTreeMap btree);

/**
 * Carrega a BTree de um arquivo. Essa operação lê apenas o header da BTree. O
 * arquivo precisa já estar criado e possuir ao menos o header disponível.
 *
 * @param btree - referência mutável da btree que terá o arquivo vinculado.
 * @param fname - nome do arquivo a ser vinculado a essa btree.
 * @return `BTREE_OK` em caso de sucesso e `BTREE_FAIL` em caso de erro. No
 *         segundo caso, uma mensagem de erro estará disponível.
 */
BTreeResult btree_load(BTreeMap *btree, const char *fname);

/**
 * Cria um arquivo de BTree e vincula ele a um `BTreeMap`.
 *
 * @param btree - referência mutável da btree que terá o arquivo vinculado.
 * @param fname - nome do arquivo a ser criado e vinculado a essa btree.
 * @return `BTREE_OK` em caso de sucesso e `BTREE_FAIL` em caso de erro. No
 *         segundo caso, uma mensagem de erro estará disponível.
 */
BTreeResult btree_create(BTreeMap *btree, const char *fname);

/**
 * Acessa um valor dado uma chave. Esse processo não envolve escritas ao disco,
 * entretanto muda o *file_pointer* da `btree` e por isso ela não se mantém
 * constante.
 *
 * @param btree - a btree a ser utilizada que precisa ter um arquivo vinculado.
 * @param key - a chave de busca.
 * @return o valor associado à `key` caso `key` esteja contida na btree e -1
 *         caso contrário. Em caso de erro, -1 é retornado e `btree_has_error()`
 *         retorna `true`.
 */
int64_t btree_get(BTreeMap *btree, int32_t key);

/**
 * Insere um par chave-valor na BTree. Assume que `btree` já possua algum
 * arquivo vinculado.
 *
 * @param btree - a btree no qual inserir.
 * @param key - a chave usada para ordenação da btree.
 * @param value - o valor associado.
 * @return `BTREE_OK` em caso de sucesso e `BTREE_FAIL` em caso de erro. No
 *         segundo caso, uma mensagem de erro estará disponível.
 */
BTreeResult btree_insert(BTreeMap *btree, int32_t key, uint64_t value);

/**
 * Verifica se a `btree` possui algum erro registrado.
 *
 * @param btree - a btree a ser verificada.
 * @return `false` caso não tenha ocorrido erro e `true` caso tenha.
 */
bool btree_has_error(BTreeMap *btree);

/**
 * Recupera a mensagem de erro da `btree`. A string retornada não deve ser
 * modificada ou liberada.
 *
 * @param btree - a btree com erro.
 * @return uma string contendo a mensagem de erro.
 */
const char *btree_get_error(BTreeMap *btree);

/**
 * Imprime os conteúdos da `btree`.
 * ATENÇÃO: Essa função causará undefined behavior caso possua um nível com
 * mais de 10000 nós.
 *
 * @param btree - a btree a ser impressa. Essa função não causa escritas ao
 *                disco, mas utiliza de `fseek` o que modifica `btree`.
 */
void btree_print(BTreeMap *btree);

#endif
