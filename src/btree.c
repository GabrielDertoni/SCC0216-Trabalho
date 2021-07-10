#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <utils.h>
#include <btree.h>

#define PAGE_SZ  ((uint64_t)77)
#define ENTRY_SZ 12

// Para essa implementação em particular temos que assumir que `CAPACITY` é um
// número ímpar.
#define CAPACITY 5

// Macro simples para prevenir repetição no código
#define ASSERT(expr) \
    if (!(expr)) return false

// Precisamos desse valor como uma variável para poder utilizar o endereço na
// função `fwrite`.
const uint64_t NULL_RRN = -1;

// Um par chave-valor.
typedef struct {
    int32_t  key;
    uint64_t value;
} Entry;

// Representa um nó da Btree.
typedef struct {
    bool     is_leaf;
    uint32_t len;
    uint32_t rrn;

    // Apenas CAPACITY - 1 espaços serão realmente ocupados no campo `entries`.
    // O espaço extra é pra podermos inserir uma `Entry` a mais de maneira
    // ordenada e assim escolher facilmente `Entry` do meio para ser promovida.
    // O mesmo vale para o campo `children`.
    uint32_t children[CAPACITY + 1];
    Entry    entries[CAPACITY];
} Node;

// Mesmo que `error` mas funciona com argumentos variáveis
static void verror(BTreeMap *btree, const char *format, va_list ap) {
    if (btree->error_msg) free(btree->error_msg);
    btree->error_msg = alloc_vsprintf(format, ap);
}

// Coloca uma determinada mensagem de erro na `btree`. O formato dos argumentos
// de formatação é o mesmo da função `printf`.
static void error(BTreeMap *btree, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    verror(btree, format, ap);
    va_end(ap);
}

// Garante que `fp` estará posicionado em determinado byte offset.
static inline void position(FILE *fp, uint64_t offset) {
    if (ftell(fp) != offset)
        fseek(fp, offset, SEEK_SET);
}

// Garante que `fp` estará posicionado numa determinada página de memória de
// acordo com o `rrn`. Vale notar que o RRN 0 não se refere ao byte offset 0.
static inline void position_rrn(FILE *fp, uint32_t rrn) {
    uint64_t offset = PAGE_SZ + PAGE_SZ * (uint64_t)rrn;
    position(fp, offset);
}

static bool read_header(BTreeMap *btree) {
    position(btree->fp, 0);

    char status;
    ASSERT(fread(&status, sizeof(char), 1, btree->fp));
    ASSERT(status == '1');
    ASSERT(fread(&btree->rrn_root, sizeof( int32_t), 1, btree->fp));
    ASSERT(fread(&btree->next_rrn, sizeof(uint32_t), 1, btree->fp));

    return true;
}

static bool read_node(BTreeMap *btree, uint32_t rrn, Node *to_read) {
    position_rrn(btree->fp, rrn);

    char is_leaf;
    ASSERT(fread(&is_leaf, sizeof(char), 1, btree->fp));

    // Converte de char para booleano.
    to_read->is_leaf = is_leaf == '1';

    ASSERT(fread(&to_read->len, sizeof(uint32_t), 1, btree->fp));
    ASSERT(fread(&to_read->rrn, sizeof(uint32_t), 1, btree->fp));

    if (!to_read->is_leaf) {
        // Se não for uma folha, temos a garantia de que há ao menos um nó filho.
        ASSERT(fread(&to_read->children[0], sizeof(uint32_t), 1, btree->fp));
    } else {
        fseek(btree->fp, sizeof(uint32_t), SEEK_CUR);
    }

    for (int i = 0; i < to_read->len; i++) {
        ASSERT(fread(&to_read->entries[i].key  , sizeof( int32_t), 1, btree->fp));
        ASSERT(fread(&to_read->entries[i].value, sizeof(uint64_t), 1, btree->fp));

        if (!to_read->is_leaf) {
            ASSERT(fread(&to_read->children[i + 1] , sizeof(uint32_t), 1, btree->fp));
        } else {
            fseek(btree->fp, sizeof(uint32_t), SEEK_CUR);
        }
    }

    return true;
}

static bool write_header(BTreeMap *btree, char status) {
    position(btree->fp, 0);
    ASSERT(fwrite(&status         , sizeof(char)    , 1, btree->fp));
    ASSERT(fwrite(&btree->rrn_root, sizeof( int32_t), 1, btree->fp));
    ASSERT(fwrite(&btree->next_rrn, sizeof(uint32_t), 1, btree->fp));

    int n_written = sizeof(char) + sizeof(int32_t) + sizeof(uint32_t);

    for (int i = n_written; i < PAGE_SZ; i++) {
        ASSERT(fwrite("@", sizeof(char), 1, btree->fp));
    }

    return true;
}

/**
 * Cria um novo `BtreeMap`. Não envolve alocação ou abertura de arquivos.
 *
 * @return uma BTree ainda sem arquivo vinculado.
 */
BTreeMap btree_new() {
    return (BTreeMap) {
        .fp        = NULL,
        .error_msg = NULL,
        .rrn_root  = -1,
        .next_rrn  = 0,
    };
}

/**
 * Libera o `BTreeMap` inclusive fechando algum arquivo vinculado.
 *
 * @param btree - a btree a ser liberada.
 */
void btree_drop(BTreeMap btree) {
    if (btree.fp) {
        // Antes de fechar o arquivo, escreve novamente o header da btree, agora
        // com status '1'.
        write_header(&btree, '1');
        fclose(btree.fp);
    }

    if (btree.error_msg)
        free(btree.error_msg);
}

/**
 * Carrega a BTree de um arquivo. Essa operação lê apenas o header da BTree. O
 * arquivo precisa já estar criado e possuir ao menos o header disponível.
 *
 * @param btree - referência mutável da btree que terá o arquivo vinculado.
 * @param fname - nome do arquivo a ser vinculado a essa btree.
 * @return `BTREE_OK` em caso de sucesso e `BTREE_FAIL` em caso de erro. No
 *         segundo caso, uma mensagem de erro estará disponível.
 */
BTreeResult btree_load(BTreeMap *btree, const char *fname) {
    FILE *fp = fopen(fname, "rb+");

    if (!fp) {
        error(btree, "failed to open file %s", fname);
        return BTREE_FAIL;
    }

    btree->fp = fp;

    // Lê somente o header da BTree.
    if (!read_header(btree)) {
        error(btree, "unable to read btree header from file");
        return BTREE_FAIL;
    }

    return BTREE_OK;
}

/**
 * Cria um arquivo de BTree e vincula ele a um `BTreeMap`.
 *
 * @param btree - referência mutável da btree que terá o arquivo vinculado.
 * @param fname - nome do arquivo a ser criado e vinculado a essa btree.
 * @return `BTREE_OK` em caso de sucesso e `BTREE_FAIL` em caso de erro. No
 *         segundo caso, uma mensagem de erro estará disponível.
 */
BTreeResult btree_create(BTreeMap *btree, const char *fname) {
    FILE *fp = fopen(fname, "wb+");

    if (!fp) {
        error(btree, "failed to create file %s", fname);
        return BTREE_FAIL;
    }

    btree->fp = fp;

    if (!write_header(btree, '0')) {
        error(btree, "failed to create header in file %s", fname);
        return BTREE_FAIL;
    }

    return BTREE_OK;
}

/**
 * Verifica se a `btree` possui algum erro registrado.
 *
 * @param btree - a btree a ser verificada.
 * @return `false` caso não tenha ocorrido erro e `true` caso tenha.
 */
bool btree_has_error(BTreeMap *btree) {
    return btree->error_msg;
}

/**
 * Recupera a mensagem de erro da `btree`. A string retornada não deve ser
 * modificada ou liberada.
 *
 * @param btree - a btree com erro.
 * @return uma string contendo a mensagem de erro.
 */
const char *btree_get_error(BTreeMap *btree) {
    return btree->error_msg;
}

// Encontra a posição onde `key` poderia ser inserida em `node`.
static int key_position(Node node, uint32_t key) {
    int i;
    for (i = 0; i < node.len && key > node.entries[i].key; i++);
    return i;
}

// Função interna para obter um valor dado uma chave.
static int64_t get(BTreeMap *btree, Node node, uint32_t key) {
    // Primeiro, encontra a posição onde `key` seria inserida no nó atual.
    int i = key_position(node, key);

    // Se tivermos encontrado a chave procurada, retornamos o valor
    // correspondente a ela.
    if (i < node.len && node.entries[i].key == key) {
        return node.entries[i].value;
    }

    // Se for um nó interno, lemos o nó filho onde `key` possa ser encontrada e
    // chamamos `get` recursivamente no nó filho.
    if (!node.is_leaf) {
        Node next;

        if (!read_node(btree, node.children[i], &next)) {
            error(btree, "failed to read node with RRN %d", node.children[i]);
            return -1;
        }

        return get(btree, next, key);
    }

    // `node` é necessáriamente um nó folha, mas não encontramos `key`,
    // portanto, retornamos -1 sinalizando que `key` não foi encontrada.
    return -1;
}

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
int64_t btree_get(BTreeMap *btree, int32_t key) {
    // Se a btree não possui arquivo vinculado, erro.
    if (!btree->fp) {
        error(btree, "no associated file");
        return -1;
    }

    // Se não há nó raiz na btree, ela está vazia e `key` não estará contida nela.
    if (btree->rrn_root < 0) {
        return -1;
    }

    // Lemos o nó raiz e chamamos a função `get` interna.
    Node root;
    if (!read_node(btree, btree->rrn_root, &root)) {
        error(btree, "unable to read root");
        return -1;
    }

    return get(btree, root, key);
}

// Cria um `Node` contendo uma único par chave-valor.
static Node node_from_entry(Entry entry, uint32_t rrn) {
    return (Node) {
        .is_leaf = true,
        .len     = 1,
        .rrn     = rrn,
        .entries = { entry },
    };
}

// Escreve um `Node` para o disco de acordo com o seu RRN.
static bool write_node(BTreeMap *btree, Node node)  {
    position_rrn(btree->fp, node.rrn);

    char is_leaf = node.is_leaf ? '1' : '0';
    ASSERT(fwrite(&is_leaf , sizeof(char), 1, btree->fp));
    ASSERT(fwrite(&node.len, sizeof(uint32_t), 1, btree->fp));
    ASSERT(fwrite(&node.rrn, sizeof(uint32_t), 1, btree->fp));

    if (!node.is_leaf) {
        // Se não for uma folha, temos a garantia de que há ao menos um nó filho.
        ASSERT(fwrite(&node.children[0], sizeof(uint32_t), 1, btree->fp));
    } else {
        ASSERT(fwrite(&NULL_RRN, sizeof(uint32_t), 1, btree->fp));
    }

    // Iteramos por todos os pares chave-valor bem como pelos RRNs dos nós
    // filhos, quando algum valor não está definido, escrevemos apenas 1s.
    for (int i = 0; i < CAPACITY - 1; i++) {
        if (i < node.len) {
            ASSERT(fwrite(&node.entries[i].key  , sizeof( int32_t), 1, btree->fp));
            ASSERT(fwrite(&node.entries[i].value, sizeof(uint64_t), 1, btree->fp));
        } else {
            ASSERT(fwrite(&NULL_RRN, sizeof( int32_t), 1, btree->fp));
            ASSERT(fwrite(&NULL_RRN, sizeof(uint64_t), 1, btree->fp));
        }

        if (!node.is_leaf && i < node.len) {
            ASSERT(fwrite(&node.children[i + 1], sizeof(uint32_t), 1, btree->fp));
        } else {
            ASSERT(fwrite(&NULL_RRN, sizeof(uint32_t), 1, btree->fp));
        }
    }

    return true;
}

// Cria e escreve um nó folha contendo apenas um par chave-valor no próximo RRN
// disponível.
static bool write_next_leaf(BTreeMap *btree, Entry entry) {
    Node leaf = node_from_entry(entry, btree->next_rrn++);
    return write_node(btree, leaf);
}

// Insere um par chave-valor num nó folha numa determinada posição do nó. Essa
// função assume que `node->is_leaf` é `true`.
static void insert_entry_leaf(Node *node, Entry entry, int at) {
    // Se a inserção deve ser feita no meio do nó, shifta todos os registros
    // e insere.
    if (at < node->len) {
        memmove(&node->entries[at + 1], &node->entries[at], (node->len - at) * sizeof(Entry));
    }

    node->entries[at] = entry;
    node->len++;
}

// Insere um par chave-valor e também o RRN do nó com elementos maiores do que
// `entry.key` num nó interno numa determinada posição do nó. Essa função assume
// que `node->is_leaf` é `false`.
static void insert_entry_inner(Node *node, Entry entry, uint32_t right_rrn, int at) {
    // Se a inserção deve ser feita no meio do nó, shifta todos os registros
    // e insere.
    if (at < node->len) {
        memmove(&node->entries[at + 1] , &node->entries[at]     , (node->len - at) * sizeof(Entry));
        memmove(&node->children[at + 2], &node->children[at + 1], (node->len - at) * sizeof(uint32_t));
    }

    node->entries[at] = entry;
    node->children[at + 1] = right_rrn;
    node->len++;
}

// Um tipo interno que serve somente para representar o comportamento de uma
// inserção num nó filho. Esse tipo auxiliar ajuda nas funções de inserção.
typedef struct {
    // O tipo de inserção que ocorreu
    enum {
        INSERT_SPLIT,
        INSERT_REPLACE,
        INSERT_FIT,
        INSERT_FAIL,
    } type;
    // A `entry` promovida, somente disponível quando `type = INSERT_SPLIT`
    Entry entry;
    // RRN do nó à direita do nó promovido, somente disponível quando `type = INSERT_SPLIT`
    uint32_t rrn;
} InsertResult;

// Funções que ajudam a criar o tipo `InsertResult`.

static inline InsertResult insertion_fail() {
    return (InsertResult){ .type = INSERT_FAIL };
}

static inline InsertResult insertion_split(Entry entry, uint32_t rrn) {
    return (InsertResult){ .type = INSERT_SPLIT, .entry = entry, .rrn = rrn };
}

static inline InsertResult insertion_fit() {
    return (InsertResult){ .type = INSERT_FIT };
}

static inline InsertResult insertion_replaced(Entry entry) {
    return (InsertResult){ .type = INSERT_REPLACE, .entry = entry };
}

// Divide um nó em dois. Cria um novo nó com as chaves maiores do que a chave
// do meio. Retorna o RRN do nó criado e também a `entry` promovida.
static InsertResult node_split(BTreeMap *btree, Node left, Entry entry) {
    assert(left.len == CAPACITY);

    Entry promoted = left.entries[CAPACITY / 2];

    Node right = {
        // O nó a ser criado será um nó folha somente se o nó a ser dividido
        // também for um nó folha.
        .is_leaf = left.is_leaf,
        .len     = CAPACITY / 2,
        .rrn     = btree->next_rrn++,
    };

    memcpy(right.entries, &left.entries[CAPACITY / 2] + 1, (CAPACITY / 2) * sizeof(Entry));

    // Se o nó não é uma folha, copia também a metade superior dos filhos do nó
    // sendo dividido.
    if (!right.is_leaf) {
        memcpy(right.children, &left.children[(CAPACITY + 1) / 2], ((CAPACITY + 1) / 2) * sizeof(uint32_t));
    }

    // O nó sendo dividido passa a ter a metade da capacidade anterior.
    left.len = CAPACITY / 2;

    // Escreve ambos os nós no disco, tanto o novo nó, quanto o nó já existente
    // atualizado.
    if (!write_node(btree, left) || !write_node(btree, right)) {
        error(btree, "failed to split node when inserting entry with key %d", entry.key);
        return insertion_fail();
    }

    // Como houve um split, retorna o par chave-valor promovido bem como o RRN
    // do novo nó criado.
    return insertion_split(promoted, right.rrn);
}

// Insere um novo par chave-valor na BTree. Essa função assume que o nó raiz já
// está criado.
static InsertResult insert(BTreeMap *btree, Node head, Entry entry) {
    int i = key_position(head, entry.key);

    if (head.is_leaf) {
        // É um nó folha, mas ainda possui espaço disponível -> insere
        insert_entry_leaf(&head, entry, i);

        if (head.len == CAPACITY) {
            // É um nó folha que não possui espaço livre -> split
            return node_split(btree, head, entry);
        }

        // Atualiza o nó no disco
        if (!write_node(btree, head)) {
            error(btree, "failed to write node when inserting entry inplace with key %d", entry.key);
            return insertion_fail();
        }

        return insertion_fit();
    }

    // É um nó interno.

    if (i < head.len && head.entries[i].key == entry.key) {
        // Encontramos outro nó com a mesma chave -> substitui, retorna a anterior.
        Entry old = head.entries[i];
        head.entries[i] = entry;

        if (!write_node(btree, head)) {
            error(btree, "failed to replace node entry with key %d", entry.key);
            return insertion_fail();
        }
        // Sinaliza que a chave antiga foi substituída.
        return insertion_replaced(old);
    }

    // É um nó interno -> redireciona para o nó filho.
    Node node;
    uint32_t node_rrn = head.children[i];

    // Lê o nó filho do disco.
    if (!read_node(btree, node_rrn, &node)) {
        error(btree, "failed to read node at RRN %d", node_rrn);
        return insertion_fail();
    }

    // Chama a função recursivamente. `result` armazena informações sobre como
    // foi feita a inserção no nó filho.
    InsertResult result = insert(btree, node, entry);

    // Se não houve split, repasse o mesmo resultado.
    if (result.type != INSERT_SPLIT) return result;

    // O nó filho deu split.
    Entry promoted = result.entry;
    uint32_t promoted_child = result.rrn;

    // Insere o par chave-valor promovidos no nó atual.
    insert_entry_inner(&head, promoted, promoted_child, i);

    if (head.len == CAPACITY) {
        // Não há mais espaço nesse nó -> split
        return node_split(btree, head, promoted);
    }

    // Ainda há espaço nesse nó -> adiciona entrada
    if (!write_node(btree, head)) {
        error(btree, "failed to write node when promoting entry inplace with key %d", promoted.key);
        return insertion_fail();
    }

    return insertion_fit();
}

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
BTreeResult btree_insert(BTreeMap *btree, int32_t key, uint64_t value) {
    Entry entry = {
        .key = key,
        .value = value,
    };

    // Se ainda não houver um nó raiz, crie ele.
    if (btree->rrn_root < 0) {
        btree->rrn_root = btree->next_rrn;
        if (!write_next_leaf(btree, entry)) {
            error(btree, "failed to write root node at RRN %d", btree->next_rrn);
            return BTREE_FAIL;
        }

        return BTREE_OK;
    }

    Node root;

    // Lê o nó raiz.
    if (!read_node(btree, btree->rrn_root, &root)) {
        error(btree, "failed to read root node at RRN %d", btree->rrn_root);
        return BTREE_FAIL;
    }

    // Insere o par chave-valor na btree.
    InsertResult result = insert(btree, root, entry);

    // Se o nó raiz deu split, cria uma nova raiz com o par chave-valor
    // promovido.
    if (result.type == INSERT_SPLIT) {
        Node new_root = {
            .is_leaf  = false,
            .len      = 1,
            .rrn      = btree->next_rrn++,
            .entries  = { result.entry },
            .children = { root.rrn, result.rrn },
        };

        if (!write_node(btree, new_root)) {
            error(btree, "failed to write new root node at RRN %d", new_root.rrn);
            return BTREE_FAIL;
        }

        btree->rrn_root = new_root.rrn;
    }

    if (result.type == INSERT_FAIL) {
        return BTREE_FAIL;
    }

    return BTREE_OK;
}

/* Funcionalidades adicionais */

// Imprime um único nó da árvore.
static void print_node(Node node) {
    printf("[%d]{ %d: %ld", node.rrn, node.entries[0].key, node.entries[0].value);
    for (int i = 1; i < node.len; i++) {
        printf(", %d: %ld", node.entries[i].key, node.entries[i].value);
    }
    printf(" }");
}

/**
 * Imprime os conteúdos da `btree`.
 * ATENÇÃO: Essa função causará undefined behavior caso possua um nível com
 * mais de 10000 nós.
 *
 * @param btree - a btree a ser impressa. Essa função não causa escritas ao
 *                disco, mas utiliza de `fseek` o que modifica `btree`.
 */
void btree_print(BTreeMap *btree) {

#define Q_SZ 10000

#define PUSH(val)                   \
    do {                            \
        queue[end] = (val);         \
        end = (end + 1) % Q_SZ;     \
    } while (0)

#define POP(var)                    \
    do {                            \
        (var) = queue[start];       \
        start = (start + 1) % Q_SZ; \
    } while (0)

    int32_t queue[Q_SZ];
    uint32_t start = 0;
    uint32_t end = 0;

    if (btree->rrn_root < 0) {
        printf("[ empty ]\n");
        return;
    }

    PUSH(btree->rrn_root);
    PUSH(-1);

    while (start != end) {
        int32_t rrn;
        POP(rrn);

        // Marca que chegamos no fim de um nível da árvore.
        if (rrn == -1) {
            printf("\n");
            if (start != end) PUSH(-1);
            continue;
        }

        Node node;

        if (!read_node(btree, rrn, &node)) {
            fprintf(stderr, "Print Error: failed to read node at RRN %d\n", rrn);
            return;
        }

        print_node(node);
        printf(" ");

        if (!node.is_leaf) {
            for (int i = 0; i < node.len + 1; i++) {
                PUSH(node.children[i]);
            }
        }
    }

#undef Q_SZ
#undef PUSH
#undef POP

}
