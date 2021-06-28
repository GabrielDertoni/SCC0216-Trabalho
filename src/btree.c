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

#define ASSERT_OK(val) \
    do { \
        CSVResult __tmp = (val); \
        if (__tmp != CSV_OK) return __tmp; \
    } while(0)

#define ASSERT(expr) \
    if (!(expr)) return false

// Precisamos desse valor como uma variável para poder utilizar o endereço na
// função `fwrite`.
const uint64_t NULL_RRN = -1;

// Uma
typedef struct {
    int32_t  key;
    uint64_t value;
} Entry;

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

// Set csv `error_msg` to some format with variable arguments list.
static void verror(BTreeMap *btree, const char *format, va_list ap) {
    if (btree->error_msg) free(btree->error_msg);
    btree->error_msg = alloc_vsprintf(format, ap);
}

static void error(BTreeMap *btree, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    verror(btree, format, ap);
    va_end(ap);
}

static inline void position(FILE *fp, uint64_t offset) {
    if (ftell(fp) != offset)
        fseek(fp, offset, SEEK_SET);
}

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

BTreeMap btree_new() {
    return (BTreeMap) {
        .fp        = NULL,
        .error_msg = NULL,
        .rrn_root  = -1,
        .next_rrn  = 0,
    };
}

void btree_drop(BTreeMap btree) {
    if (btree.fp) {
        write_header(&btree, '1');
        fclose(btree.fp);
    }

    if (btree.error_msg)
        free(btree.error_msg);
}

BTreeResult btree_load(BTreeMap *btree, const char *fname) {
    FILE *fp = fopen(fname, "rb+");

    if (!fp) {
        error(btree, "failed to open file %s", fname);
        return BTREE_FAIL;
    }

    btree->fp = fp;

    if (!read_header(btree)) {
        error(btree, "unable to read btree header from file");
        return BTREE_FAIL;
    }

    return BTREE_OK;
}

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

bool btree_has_error(BTreeMap *btree) {
    return btree->error_msg;
}

static int key_position(Node node, uint32_t key) {
    int i;
    for (i = 0; i < node.len && key > node.entries[i].key; i++);
    return i;
}

static int64_t get(BTreeMap *btree, Node node, uint32_t key) {
    int i = key_position(node, key);

    if (i < node.len && node.entries[i].key == key) {
        return node.entries[i].value;
    }

    if (!node.is_leaf) {
        Node next;

        if (!read_node(btree, node.children[i], &next)) {
            error(btree, "failed to read node with RRN %d", node.children[i]);
            return -1;
        }

        return get(btree, next, key);
    }

    return -1;
}

int64_t btree_get(BTreeMap *btree, uint32_t key) {
    if (btree->rrn_root < 0) {
        return -1;
    }

    Node root;
    if (!read_node(btree, btree->rrn_root, &root)) {
        error(btree, "unable to read root");
        return -1;
    }

    return get(btree, root, key);
}

static Node node_from_entry(Entry entry, uint32_t rrn) {
    return (Node) {
        .is_leaf = true,
        .len     = 1,
        .rrn     = rrn,
        .entries = { entry },
    };
}

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

static bool write_next_leaf(BTreeMap *btree, Entry entry) {
    Node leaf = node_from_entry(entry, btree->next_rrn++);
    return write_node(btree, leaf);
}

static void insert_entry_leaf(Node *node, Entry entry, int at) {
    // Se a inserção deve ser feita no meio do nó, shifta todos os registros
    // e insere.
    if (at < node->len) {
        memmove(&node->entries[at + 1], &node->entries[at], (node->len - at) * sizeof(Entry));
    }

    node->entries[at] = entry;
    node->len++;
}

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

// Um tipo interno que serve para somente para representar o comportamento de
// uma inserção num nó filho.
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
        .is_leaf = left.is_leaf,
        .len     = CAPACITY / 2,
        .rrn     = btree->next_rrn++,
    };

    memcpy(right.entries, &left.entries[CAPACITY / 2] + 1, (CAPACITY / 2) * sizeof(Entry));

    if (!right.is_leaf) {
        memcpy(right.children, &left.children[(CAPACITY + 1) / 2], ((CAPACITY + 1) / 2) * sizeof(uint32_t));
    }

    left.len = CAPACITY / 2;

    if (!write_node(btree, left) || !write_node(btree, right)) {
        error(btree, "failed to split node when inserting entry with key %d", entry.key);
        return insertion_fail();
    }

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

        // TODO: Podería atualizar somente a entry que mudou e não o nó inteiro.
        if (!write_node(btree, head)) {
            error(btree, "failed to replace node entry with key %d", entry.key);
            return insertion_fail();
        }
        return insertion_replaced(old);
    }

    // É um nó interno -> redireciona para o nó filho.
    Node node;
    uint32_t node_rrn = head.children[i];

    if (!read_node(btree, node_rrn, &node)) {
        error(btree, "failed to read node at RRN %d", node_rrn);
        return insertion_fail();
    }

    InsertResult result = insert(btree, node, entry);

    if (result.type != INSERT_SPLIT) return result;

    Entry promoted = result.entry;
    uint32_t promoted_child = result.rrn;

    // O nó filho deu split.
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

// Insere um novo par chave-valor na BTree. Cria um nó raiz caso ele não exista.
// Essa função atua como um wrapper para a função interna `insert`.
BTreeResult btree_insert(BTreeMap *btree, int32_t key, uint64_t value) {
    Entry entry = {
        .key = key,
        .value = value,
    };

    if (btree->rrn_root < 0) {
        btree->rrn_root = btree->next_rrn;
        if (!write_next_leaf(btree, entry)) {
            error(btree, "failed to write root node at RRN %d", btree->next_rrn);
            return BTREE_FAIL;
        }

        return BTREE_OK;
    }

    Node root;

    if (!read_node(btree, btree->rrn_root, &root)) {
        error(btree, "failed to read root node at RRN %d", btree->rrn_root);
        return BTREE_FAIL;
    }

    InsertResult result = insert(btree, root, entry);

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

static void print_node(Node node) {
    printf("[%d]{ %d: %ld", node.rrn, node.entries[0].key, node.entries[0].value);
    for (int i = 1; i < node.len; i++) {
        printf(", %d: %ld", node.entries[i].key, node.entries[i].value);
    }
    printf(" }");
}

// Imprime os conteúdos de uma BTree. ATENÇÃO: Essa função causará undefined
// behaviour caso possua um nível com mais de 10000 nós.
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
