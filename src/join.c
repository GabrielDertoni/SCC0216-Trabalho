#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <common.h>
#include <utils.h>
#include <bin.h>
#include <btree.h>

// Verifica se o arquivo existe, retorna true se sim, e false ao contrário.
static bool check_file(FILE *fp){
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
    if(!check_file(file_vehicle)) return false;
    if(!check_file(file_busline)) return false;

    DBVehicleHeader header_vehicle;
    if (!read_header_vehicle(file_vehicle, &header_vehicle)) {
        printf(ERROR_FOUND);
        fclose(file_busline);
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
    for (int i = 0; i < n_vehicle_registers; i++){
        // Lê e verifica erros na leitura do registro de veículo.
        if (!read_vehicle_register(file_vehicle, &reg_vehicle)) {
            printf(ERROR_FOUND);
            fclose(file_busline);
            fclose(file_vehicle);
            return false;
        }

        // Sets the bus line binary file to the start
        fseek(file_busline, BUS_LINE_HEADER_SIZE, SEEK_SET);

        // Checks if the current vehicle register is removed
        if (reg_vehicle.removido == '0')
            continue;

        // Loops and reads all the registers in the binary bus line file
        for (int j = 0; j < n_busline_registers; j++){
            if (!read_bus_line_register(file_busline, &reg_busline)) {
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
    if(!check_file(file_vehicle)) return false;
    if(!check_file(file_busline)) return false;

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
bool merge_sorted(const char *vehicle_bin_fname, const char *busline_bin_fname) {
    // Cria os nomes dos arquivos ordenados como sendo o mesmo nome dos arquivos
    // originais mas com o sufixo "_ordenado".
    char *sorted_vehicle_bin_fname = alloc_sprintf("%s_ordenado", vehicle_bin_fname);
    char *sorted_busline_bin_fname = alloc_sprintf("%s_ordenado", busline_bin_fname);

    // Chama as funcionalidades que ordenam os arquivos de linha e veículo.
    if (!order_vehicle_bin_file(vehicle_bin_fname, sorted_vehicle_bin_fname) ||
        !order_bus_line_bin_file(busline_bin_fname, sorted_busline_bin_fname))
    {
        return false;
    }
    
    // Tenta abrir os dois arquivos gerados pelas funcionalidades de ordenação e
    // libera os nomes que foram alocados dinamicamente.

    FILE *sorted_vehicle_fp = fopen(sorted_vehicle_bin_fname, "rb");
    free(sorted_vehicle_bin_fname);

    if (!check_file(sorted_vehicle_fp)) return false;

    FILE *sorted_busline_fp = fopen(sorted_busline_bin_fname, "rb");
    free(sorted_busline_bin_fname);

    if (!check_file(sorted_busline_fp)) {
        fclose(sorted_vehicle_fp);
        return false;
    }

    // Carrega o header de cada um dos arquivos binários, sempre verificando se
    // ocorreu algum erro.

    DBVehicleHeader header_vehicle;
    DBBusLineHeader header_busline;
    if (!read_header_vehicle(sorted_vehicle_fp, &header_vehicle) ||
        !read_header_bus_line(sorted_busline_fp, &header_busline))
    {
        printf(ERROR_FOUND);
        fclose(sorted_vehicle_fp);
        fclose(sorted_busline_fp);
        return false;
    }

    DBBusLineRegister reg_busline;
    DBVehicleRegister reg_vehicle;

    // Os arquivos binários ordenados não possuem registros removidos, então o
    // número total de registros é simplesmente `nroRegistros`.

    uint32_t n_vehicle_registers = header_vehicle.meta.nroRegistros;
    uint32_t n_busline_registers = header_busline.meta.nroRegistros;

    uint32_t vehicle_count = 0;
    uint32_t busline_count = 0;

    uint32_t n_matching = 0;

    memset(&reg_vehicle, 0, sizeof(DBVehicleRegister));
    memset(&reg_busline, 0, sizeof(DBBusLineRegister));

    while (vehicle_count < n_vehicle_registers && busline_count <= n_busline_registers) {
        vehicle_drop(reg_vehicle);
        read_vehicle_register(sorted_vehicle_fp, &reg_vehicle);
        vehicle_count++;

        while (busline_count < n_busline_registers && reg_busline.codLinha < reg_vehicle.codLinha) {
            bus_line_drop(reg_busline);
            read_bus_line_register(sorted_busline_fp, &reg_busline);
            busline_count++;
        }

        if (reg_vehicle.codLinha == reg_busline.codLinha) {
            n_matching++;
            print_vehicle(stdout, &reg_vehicle, &header_vehicle);
            print_bus_line(stdout, &reg_busline, &header_busline);
            printf("\n");
        }
    }

    vehicle_drop(reg_vehicle);
    bus_line_drop(reg_busline);

    fclose(sorted_vehicle_fp);
    fclose(sorted_busline_fp);

    if (n_matching == 0) {
        printf(NO_REGISTER);
        return false;
    }
    return true;
}
