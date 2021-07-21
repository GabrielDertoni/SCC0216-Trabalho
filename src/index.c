#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <external.h>
#include <index.h>
#include <btree.h>
#include <bin.h>
#include <csv.h>
#include <parsing.h>
#include <common.h>

// Trata erros das funções que trabalham com um arquivo binário e uma btree.
// Quando compilado com -DDEBUG, imprime uma mensagem de erro descritiva, se não
// imprime simplesmente ERROR_FOUND e retorna sempre `false` para poder ser
// ergonômicamente integrada nas funções.
//
// Essa função fecha o arquivo `to_close` e libera `failed_btree`. O argumento
// `format` pode ser NULL, nesse caso apenas o erro contido em `failed_btree` é
// impresso, senão `format` junto com os outros argumentos variáveis são
// impressos seguindo o padrão `printf`.
static inline bool handle_error(FILE *to_close, BTreeMap failed_btree, const char *format, ...) {
    va_list ap;
    va_start(ap, format);

#ifdef DEBUG
    if (btree_has_error(&failed_btree)) {
        fprintf(stderr, "Error: %s.\n", btree_get_error(&failed_btree));
    }

    if (format) {
        fprintf(stderr, "Error: ");
        vfprintf(stderr, format, ap);
        fprintf(stderr, ".\n");
    } else {
        fprintf(stderr, "Error: unexpected.\n");
    }
#else
    printf(ERROR_FOUND);
#endif

    btree_drop(failed_btree);
    if(to_close)
        fclose(to_close);

    va_end(ap);

    return false;
}

/*
* Cria um arquivo de indice arvore-B para o arquivo de dados veiculo
* @params bin_fname - nome do arquivo binario veiculos
* @params index_fname - nome do arquivo binario de indice arvore-B
* @returns um valor booleano - true se for criado, false se der algum erro
*/
bool index_vehicle_create(const char *bin_fname, const char *index_fname) {
    BTreeMap btree = btree_new();

    FILE *bin_fp = fopen(bin_fname, "rb");

    if (!bin_fp)
        return handle_error(bin_fp, btree, "failed to open file %s", bin_fname);

    if (btree_create(&btree, index_fname) != BTREE_OK)
        return handle_error(bin_fp, btree, NULL);

    DBVehicleHeader header;
    if (!read_header_vehicle(bin_fp, &header))
        return handle_error(bin_fp, btree, "failed to read vehicle header from %s", bin_fname);

    DBVehicleRegister reg;

    uint32_t total_register = header.meta.nroRegistros + header.meta.nroRegRemovidos;
    uint64_t offset = ftell(bin_fp);

    // Le todos os registros de veiculos do arquivo binario e se nao estiver marcado como removido os insere na arvore-B
    for (int i = 0; i < total_register; i++){
        if (!read_vehicle_register(bin_fp, &reg))
            return handle_error(bin_fp, btree, "failed to read vehicle register");

        if (reg.removido == '1') {
            int32_t hash = convertePrefixo(reg.prefixo);
            if (btree_insert(&btree, hash, offset) != BTREE_OK)
                return handle_error(bin_fp, btree, NULL);
        }

        free(reg.modelo);
        free(reg.categoria);

        offset = ftell(bin_fp);
    }

    btree_drop(btree);
    fclose(bin_fp);

    return true;
}

/*
* Cria um arquivo de indice arvore-B para o arquivo de dados linhas de onibus
* @params bin_fname - nome do arquivo binario linhas de onibus
* @params index_fname - nome do arquivo binario de indice arvore-B
* @returns um valor booleano - true se for criado, false se der algum erro
*/
bool index_bus_line_create(const char *bin_fname, const char *index_fname) {
    BTreeMap btree = btree_new();

    FILE *bin_fp = fopen(bin_fname, "rb");

    if (!bin_fp)
        return handle_error(bin_fp, btree, "failed to open file %s", bin_fname);

    if (btree_create(&btree, index_fname) != BTREE_OK)
        return handle_error(bin_fp, btree, NULL);

    DBBusLineHeader header;
    if (!read_header_bus_line(bin_fp, &header))
        return handle_error(bin_fp, btree, "failed to read bus line header from %s", bin_fname);

    DBBusLineRegister reg;

    uint32_t total_register = header.meta.nroRegistros + header.meta.nroRegRemovidos;
    uint64_t offset = ftell(bin_fp);

    // Le todos os registros de linhas de onibus do arquivo binario e se nao estiver marcado como removido os insere na arvore-B
    for (int i = 0; i < total_register; i++){
        if (!read_bus_line_register(bin_fp, &reg))
            return handle_error(bin_fp, btree, "failed to read bus line register");

        if (reg.removido == '1') {
            if (btree_insert(&btree, reg.codLinha, offset) != BTREE_OK)
                return handle_error(bin_fp, btree, NULL);
        }

        free(reg.nomeLinha);
        free(reg.corLinha);

        offset = ftell(bin_fp);
    }

    btree_drop(btree);
    fclose(bin_fp);

    return true;
}

/*
* Recupera os regstros buscados de um determinado arquivo de dados veiculo usando o indice arvore-B
* @params bin_fname - nome do arquivo binario veiculos
* @params index_fname - nome do arquivo binario de indices arvore-B
* @params prefixo[6] - valor do campo prefixo em que sera feita a busca
* @returns um valor booleano - true se a busca for feita com sucesso, false se ocorrer algum erro
*/
bool search_for_vehicle(const char *bin_fname, const char *index_fname, const char prefixo[6]) {
    BTreeMap btree = btree_new();

    FILE *bin_fp = fopen(bin_fname, "rb");

    if (!bin_fp)
        return handle_error(bin_fp, btree, "failed to open file %s", bin_fname);

    DBVehicleHeader header;
    if (!read_header_vehicle(bin_fp, &header))
        return handle_error(bin_fp, btree, "failed to read vehicle register from %s", bin_fname);

    if (btree_load(&btree, index_fname) != BTREE_OK)
        return handle_error(bin_fp, btree, NULL);

    int32_t hash = convertePrefixo((char *)prefixo);
    int64_t off = btree_get(&btree, hash);

    if (btree_has_error(&btree))
        return handle_error(bin_fp, btree, NULL);

    // Verifica se o valor buscado contem algum resultado. Em caso positivo
    // exibe os valores buscados, em caso contrario exibe uma mensagem de erro
    if (off < 0) {
        printf(NO_REGISTER);
    } else {
        fseek(bin_fp, off, SEEK_SET);

        DBVehicleRegister reg;
        if (!read_vehicle_register(bin_fp, &reg))
            return handle_error(bin_fp, btree, "failed to read vehicle register");

        print_vehicle(stdout, &reg, &header);
        vehicle_drop(reg);
    }

    btree_drop(btree);
    fclose(bin_fp);

    return true;
}

/*
* Recupera os regstros buscados de um determinado arquivo de dados lnhas de onibus usando o indice arvore-B
* @params bin_fname - nome do arquivo binario linhas de onibus
* @params index_fname - nome do arquivo binario de indices arvore-B
* @params code - valor do campo cdigo em que sera feita a busca
* @returns um valor booleano - true se a busca for feita com sucesso, false se ocorrer algum erro
*/
bool search_for_bus_line(const char *bin_fname, const char *index_fname, uint32_t code) {
    BTreeMap btree = btree_new();

    FILE *bin_fp = fopen(bin_fname, "rb");

    if (!bin_fp)
        return handle_error(bin_fp, btree, "failed to open file %s", bin_fname);

    DBBusLineHeader header;
    if (!read_header_bus_line(bin_fp, &header))
        return handle_error(bin_fp, btree, "failed to read bus line header from %s", bin_fname);

    if (btree_load(&btree, index_fname) != BTREE_OK)
        return handle_error(bin_fp, btree, NULL);

    int64_t off = btree_get(&btree, code);

    if (btree_has_error(&btree))
        return handle_error(bin_fp, btree, NULL);

    // Verifica se o valor buscado contem algum resultado. Em caso positivo
    // exibe os valores buscados, em caso contrario exibe uma mensagem de erro
    if (off < 0) {
        printf(NO_REGISTER);
    } else {
        fseek(bin_fp, off, SEEK_SET);

        DBBusLineRegister reg;
        if (!read_bus_line_register(bin_fp, &reg))
            return handle_error(bin_fp, btree, "failed to read bus line register");

        print_bus_line(stdout, &reg, &header);
        bus_line_drop(reg);
    }

    btree_drop(btree);
    fclose(bin_fp);
    return true;
}

typedef struct {
    FILE *bin_fp;
    BTreeMap *btree;
    size_t reg_count;
    size_t removed_reg_count;
} IterArgs;

/*
* Itera sobre sobre os dados de indice dos veiculos.
* @params csv - struct do tipo CSV
* @params vehicle - struct do tipo Vehicle
* @params args - struct do tipo InterArgs
* @returns uma informacao sobre a iteracao na forma de um enum do tipo CSVResult
*/
static CSVResult vehicle_index_row_iterator(CSV *csv, const Vehicle *vehicle, IterArgs *args) {
    size_t offset = ftell(args->bin_fp);

    if (!write_vehicle(vehicle, args->bin_fp)) {
        csv_error(csv, "failed to write vehicle");
        return CSV_ERR_OTHER;
    }

    // Conta a quantidade de registros removidos
    if (vehicle->prefixo[0] == REMOVED_MARKER) {
        args->removed_reg_count++;
    } else {
        args->reg_count++;

        // Por algum motivo `convertePrefixo` recebe um argumento não `const`, então
        // precisamos desse cast.
        int32_t hash = convertePrefixo((char *)vehicle->prefixo);

        if (btree_insert(args->btree, hash, offset) != BTREE_OK) {
            csv_error(csv, "failed to insert vehicle register in index: %s",
                      btree_get_error(args->btree));
            return CSV_ERR_OTHER;
        }
    }
    return CSV_OK;
}

/*
* Itera sobre sobre os dados de indice de linhas de onibus.
* @params csv - struct do tipo CSV
* @params bus_line - struct do tipo BusLine
* @params args - struct do tipo InterArgs
* @returns uma informacao sobre a iteracao na forma de um enum do tipo CSVResult
*/
static CSVResult bus_line_index_row_iterator(CSV *csv, const BusLine *bus_line, IterArgs *args) {
    size_t offset = ftell(args->bin_fp);

    if (!write_bus_line(bus_line, args->bin_fp)) {
        csv_error(csv, "failed to write bus line");
        return CSV_ERR_OTHER;
    }

    // Conta a quantidade de registros removidos
    if (bus_line->codLinha[0] == REMOVED_MARKER) {
        args->removed_reg_count++;
    } else {
        args->reg_count++;

        int32_t codLinha = (int)strtol(bus_line->codLinha, NULL, 10);
        if (btree_insert(args->btree, codLinha, offset) != BTREE_OK) {
            csv_error(csv, "failed to insert bus line register in index: %s",
                      btree_get_error(args->btree));
            return CSV_ERR_OTHER;
        }
    }

    return CSV_OK;
}

/*
* Insere valores em um arquivo binario e um arquivo de indice
* @params bin_fname - nome do arquivo binario a ser inserido (tanto veiculos quanto linhas de onibus)
* @params index_fname - nome do arquivo binario de indices arvore-B
* @params csv - struct do tipo CSV
* @params iter - ponteiro para funcao dotipo CSVIterFunc
* @params sep - substring de separacao dos dados no arquivo
*/
static bool csv_append_to_bin_and_index(
    const char *bin_fname,
    const char *index_fname,
    CSV *csv,
    CSVIterFunc *iter,
    const char *sep
) {
    BTreeMap btree = btree_new();
    FILE *bin_fp = fopen(bin_fname, "r+b");

    if (!bin_fp)
        return handle_error(bin_fp, btree, "could not open file %s", bin_fname);

    // Verifica se a arvore-B consegue ser carregada
    if (btree_load(&btree, index_fname) != BTREE_OK)
        return handle_error(bin_fp, btree, "could not load btree from file %s", index_fname);

    DBMeta meta;

    // Verifica se o arquivo binario consegue ser lido e reailza a leitura
    if (!read_meta(bin_fp, &meta))
        return handle_error(bin_fp, btree, "could not read meta header from file %s", bin_fname);

    // Verifica se o status do arquivo binario consegue ser atualizado e o atualiza para 0
    // (siginifica que sera realizado insercao)
    if (!update_header_status('0', bin_fp))
        return handle_error(bin_fp, btree, "could not write status to file %s", bin_fname);

    IterArgs args = {
        .bin_fp            = bin_fp,
        .btree             = &btree,
        .reg_count         = 0,
        .removed_reg_count = 0,
    };

    // Vai para o fim do arquivo para adicionar novos registros.
    fseek(bin_fp, 0L, SEEK_END);

    if (csv_iterate_rows(csv, sep, (CSVIterFunc *)iter, &args) != CSV_OK)
        return handle_error(bin_fp, btree, NULL);

    meta.status = '1';
    meta.byteProxReg = ftell(bin_fp);
    meta.nroRegRemovidos += args.removed_reg_count;
    meta.nroRegistros += args.reg_count;

    if (!update_header_meta(&meta, bin_fp))
        return handle_error(bin_fp, btree, "could not write meta header to file %s", bin_fname);

    fclose(bin_fp);
    btree_drop(btree);

    return true;
}

/*
* Insere cada registro em um arquivo binário de dados veículo e a chave de busca
* correspondente a essa inserção inserida no indice arvore-B
*
* @params bin_fname - string que corresponde ao nome do arquivo binario de veiculos
* @params index_fname - string que corresponde ao nome do arquivo binario de indices arvore-B
* @returns um valor booleano - true se a insercao ocorrer e false se nao ocorrer
*/
bool csv_append_to_bin_and_index_vehicle(const char *bin_fname, const char *index_fname) {
    CSV csv = configure_vehicle_csv();
    csv_use_fp(&csv, stdin);

    bool ok = csv_append_to_bin_and_index(bin_fname, index_fname, &csv, (CSVIterFunc *)vehicle_index_row_iterator, " ");

    csv_drop(csv);
    return ok;
}

/*
* Insere cada registro em um arquivo binário de dados linha de onibus e a chave de busca
* correspondente a essa inserção inserida no indice arvore-B
*
* @params bin_fname - string que corresponde ao nome do arquivo binario de linhas de onibus
* @params index_fname - string que corresponde ao nome do arquivo binario de indices arvore-B
* @returns um valor booleano - true se a insercao ocorrer e false se nao ocorrer
*/
bool csv_append_to_bin_and_index_bus_line(const char *bin_fname, const char *index_fname) {
    CSV csv = configure_bus_line_csv();
    csv_use_fp(&csv, stdin);

    bool ok = csv_append_to_bin_and_index(bin_fname, index_fname, &csv, (CSVIterFunc *)bus_line_index_row_iterator, " ");

    csv_drop(csv);
    return ok;
}

// Verifica se o arquivo existe, retorna true se sim, e false ao contrário.
bool checks_file(FILE *fp){
    if(!fp){
        printf(ERROR_FOUND);
        return false;
    }
    return true;
}

// Verifica a quantidade de itens que satisfazem uma busca. Exibe uma mensagem de erro se
// nenhuma é encontrada e retorna false, retorna true em caso contrário.
static bool checks_matching(int n_matching){
    if(n_matching <= 0){
        printf(NO_REGISTER);
        return false;
    }
    return true;
}

// TODO - alterar meus comentários em inglês, estava com o código de grafos na cabeça ainda
/*
* Exibe os resultados que satisfazem a busca de codLinha no arquivo binário de veículos e
* no arquivo binário de linhsa de ônibus
*
* @param vehiclebin_fname - caminho para o arquivo binário de veículos
* @param buslinebin_fname - caminho para o arquivo binário de linhas de ônibus
* @returns - um valor booleano = true se a leitura dos arquivos der certo e retornar algum
*            resultado, false se a leitura dos arquivos der errado ou não retornar nenhum
*            resultado da busca
*/
bool join_vehicle_and_bus_line(const char *vehiclebin_fname, const char *buslinebin_fname){
    FILE *file_vehicle = fopen(vehiclebin_fname, "rb");
    FILE *file_busline = fopen(buslinebin_fname, "rb");

    // Verifica se os caminho dos dois arquivos existem
    if(!checks_file(file_vehicle)) return false;
    if(!checks_file(file_busline)) return false;

    DBVehicleHeader header_vehicle;
    if (!read_header_vehicle(file_vehicle, &header_vehicle)) {
        printf(ERROR_FOUND);
        fclose(file_vehicle);
        return false;
    }

    // Reads the header of the binary bus line file
    DBBusLineHeader header_busline;
    if (!read_header_bus_line(file_busline, &header_busline)) {
        printf(ERROR_FOUND);
        fclose(file_busline);
        fclose(file_vehicle);
        return false;
    }

    DBBusLineRegister reg_busline;
    DBVehicleRegister reg_vehicle;

    uint32_t n_vehicle_registers = header_vehicle.meta.nroRegistros + header_vehicle.meta.nroRegRemovidos;
    uint32_t n_busline_registers = header_busline.meta.nroRegistros + header_busline.meta.nroRegRemovidos;

    int n_matching = 0;
    // Loops and reads all the binary vehicle registers
    for (int i = 0; n_vehicle_registers; i++){
        // Lê e verifica erros na leitura do registro de veículo.
        if (!read_vehicle_register(file_vehicle, &reg_vehicle)) {
            printf(ERROR_FOUND);
            fclose(file_busline);
            fclose(file_vehicle);
            return false;
        }

        // Sets the bus line binary file to the start
        if (i > 0) {
            fseek(file_busline, BUS_LINE_HEADER_SIZE, SEEK_SET);
        }

        // Checks if the current vehicle register is removed
        if (reg_vehicle.removido == '0')
            continue;

        // Loops and reads all the registers in the binary bus line file
        for (int j = 0; j < n_busline_registers; j++){
            if (read_bus_line_register(file_busline, &reg_busline)) {
                printf(ERROR_FOUND);
                fclose(file_busline);
                fclose(file_vehicle);
                return false;
            }

            // Checks if the current bus line register is removed
            bool removed_busline = reg_busline.removido == '0';

            // Prints the result if matches the criteria
            if(reg_vehicle.codLinha == reg_busline.codLinha && !removed_busline){
                print_vehicle(stdout, &reg_vehicle, &header_vehicle);
                print_bus_line(stdout, &reg_busline, &header_busline);
                fprintf(stdout, "\n");
                n_matching++; // Adds +1 in the numberof matches
            }
        }
    }
    // Closes the binary files
    fclose(file_busline);
    fclose(file_vehicle);

    // Prints an error message if no match if found
    return checks_matching(n_matching);
}

// TODO - alterar meus comentários em inglês, estava com o código de grafos na cabeça ainda
/*
* Exibe os resultados que satisfazem a busca de codLinha no arquivo binário de veículos e
* no arquivo binário de linha de ônibus
*
* @param vehiclebin_fname - caminho para o arquivo binário de veículos
* @param buslinebin_fname - caminho para o arquivo binário de linhas de ônibus
* @param index_btree_fname - caminho para o arquivo binário de índices árvore-B
* @returns - um valor booleano = true se a leitura dos arquivos der certo e retornar algum
*            resultado, false se a leitura dos arquivos der errado ou não retornar nenhum
*            resultado da busca
*/
bool join_vehicle_and_bus_line_using_btree(
    const char *vehiclebin_fname,
    const char *buslinebin_fname,
    const char *index_btree_fname
){
    FILE *file_vehicle = fopen(vehiclebin_fname, "rb");
    FILE *file_busline = fopen(buslinebin_fname, "rb");

    // Verifica se os caminho dos dois arquivos existem
    if(!checks_file(file_vehicle)) return false;
    if(!checks_file(file_busline)) return false;

    DBVehicleRegister reg_vehicle;
    BTreeMap btree = btree_new();

    DBVehicleHeader header_vehicle;
    if (!read_header_vehicle(file_vehicle, &header_vehicle)) {
        fclose(file_busline);
        return handle_error(file_vehicle, btree, "could not read header from %s", vehiclebin_fname);
    }

    // Reads the header of the binary bus line file
    DBBusLineHeader header_busline;
    if (!read_header_bus_line(file_busline, &header_busline)) {
        fclose(file_busline);
        return handle_error(file_vehicle, btree, "could not read header from %s", vehiclebin_fname);
    }

    if (btree_load(&btree, index_btree_fname) != BTREE_OK) {
        fclose(file_busline);
        return handle_error(file_vehicle, btree, NULL);
    }

    uint32_t n_vehicle_registers = header_vehicle.meta.nroRegistros + header_vehicle.meta.nroRegRemovidos;

    int n_matching = 0;
    // Loops and reads all the binary vehicle registers
    for (int i = 0; i < n_vehicle_registers; i++){
        if (!read_vehicle_register(file_vehicle, &reg_vehicle)) {
            fclose(file_busline);
            return handle_error(file_vehicle, btree, "failed to read register from %s", vehiclebin_fname);
        }

        // Verifica se o atual registro veículo está marcado como removido. Se estiver, passa para a próxima iteração
        if(reg_vehicle.removido == '0')
            continue;

        // Sets the bus line binary file to the start
        fseek(file_busline, 0, SEEK_SET);

        int64_t off = btree_get(&btree, reg_vehicle.codLinha);
        if(btree_has_error(&btree)) {
            fclose(file_busline);
            return handle_error(file_busline, btree, NULL);
        }

        if(off >= 0){
            fseek(file_busline, off, SEEK_SET);

            DBBusLineRegister reg_busline;
            if (!read_bus_line_register(file_busline, &reg_busline)) {
                fclose(file_busline);
                return handle_error(file_vehicle, btree, "failed to read bus line register");
            }

            print_vehicle(stdout, &reg_vehicle, &header_vehicle);
            print_bus_line(stdout, &reg_busline, &header_busline);
            fprintf(stdout, "\n");
            n_matching++;
        }
    }

    // Closes the binary files
    fclose(file_busline);
    fclose(file_vehicle);

    // Prints an error message if no match if found
    return checks_matching(n_matching);
}
