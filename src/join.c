#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <common.h>
#include <utils.h>
#include <bin.h>
#include <sort.h>
#include <btree.h>

// Verifica a quantidade de itens que satisfazem uma busca. Exibe uma mensagem de erro se
// nenhuma é encontrada e retorna false, retorna true em caso contrário.
static bool checks_matching(int n_matching){
    if(n_matching <= 0){
        printf(NO_REGISTER);
        return false;
    }
    return true;
}

// Mesmo que `handle_error` mas recebe uma `va_list` ao invés de argumentos
// variádicos.
static inline bool vhandle_error(FILE *restrict to_close1, FILE *restrict to_close2, const char *format, va_list ap) {
#ifdef DEBUG
    if (format) {
        fprintf(stderr, "Error: ");
        vfprintf(stderr, format, ap);
        fprintf(stderr, ".\n");
    }
#else
    printf(ERROR_FOUND);
#endif

    if(to_close1) fclose(to_close1);
    if(to_close2) fclose(to_close2);

    return false;
}

/**
 * Função interna utilitária que fecha dois arquivos e imprime uma mensagem de
 * erro seguindo os formatos da função `printf`. Caso o programa seja compilado
 * com `-DDEBUG` imprime a mensagem passada para `stderr`, caso não, imprime o
 * conteúdo do macro `ERROR_FOUND`.
 *
 * Essa função torna a sintaxe mais direta dentro das função exportadas sem a
 * necessidade de muito código repetido. Ela sempre retorna `false` por
 * conveniência.
 *
 * @param to_close1 - um arquivo que será fechado.
 * @param to_close2 - outro arquivo que será fechado.
 * @param format - uma string de formato, igual a do `printf`.
 * @param ... - argumentos variádicos que seguem ao padrão `printf`.
 */
static inline bool handle_error(FILE *restrict to_close1, FILE *restrict to_close2, const char *format, ...) {
    va_list ap;
    va_start(ap, format);

    vhandle_error(to_close1, to_close2, format, ap);

    va_end(ap);
    return false;
}

/**
 * Mesmo que `handle_error`, mas também libera uma `BTreeMap`. Quando compilado
 * com `-DDEBUG`, caso a btree tenha um erro, imprime o erro da btree e a
 * mensagem especificada nos últimos argumentos no `stderr`.
 *
 * @param to_close1 - um arquivo que será fechado.
 * @param to_close2 - outro arquivo que será fechado.
 * @param failed_btree - btree com erro.
 * @param format - uma string de formato, igual a do `printf`.
 * @param ... - argumentos variádicos que seguem ao padrão `printf`.
 */
static inline bool handle_error_btree(
    FILE *restrict to_close1,
    FILE *restrict to_close2,
    BTreeMap failed_btree,
    const char *format,
    ...
) {
    va_list ap;
    va_start(ap, format);

#ifdef DEBUG
    if (btree_has_error(&failed_btree)) {
        fprintf(stderr, "Error: %s.\n", btree_get_error(&failed_btree));
    }
#endif
    btree_drop(failed_btree);

    vhandle_error(to_close1, to_close2, format, ap);

    va_end(ap);
    return false;
}

/**
 * Exibe os resultados que satisfazem a busca de codLinha no arquivo binário de veículos e
 * no arquivo binário de linhsa de ônibus
 *
 * @param vehicle_bin_fname - caminho para o arquivo binário de veículos
 * @param busline_bin_fname - caminho para o arquivo binário de linhas de ônibus
 * @returns - um valor booleano = true se a leitura dos arquivos der certo e retornar algum
 *            resultado, false se a leitura dos arquivos der errado ou não retornar nenhum
 *            resultado da busca
 */
bool join_vehicle_and_bus_line(const char *vehicle_bin_fname, const char *busline_bin_fname){
    // Abre os dois arquivos e verifica se foi bem sucedido

    FILE *file_vehicle = NULL;
    FILE *file_busline = NULL;

    file_vehicle = fopen(vehicle_bin_fname, "rb");
    if(!file_vehicle)
        return handle_error(file_busline, file_vehicle,
                            "could not open %s",
                            vehicle_bin_fname);

    file_busline = fopen(busline_bin_fname, "rb");
    if(!file_busline)
        return handle_error(file_busline, file_vehicle,
                            "could not open %s",
                            busline_bin_fname);

    // Lê os headers de ambos os binários e verifica se houve algum erro.
    DBVehicleHeader header_vehicle;

    if (!read_header_vehicle(file_vehicle, &header_vehicle))
        return handle_error(file_busline, file_vehicle,
                            "could not read header from %s",
                            vehicle_bin_fname);

    DBBusLineHeader header_busline;
    if (!read_header_bus_line(file_busline, &header_busline))
        return handle_error(file_busline, file_vehicle,
                            "could not read header from %s",
                            busline_bin_fname);

    // Buffers para os registros que serão lidos.
    DBBusLineRegister reg_busline;
    DBVehicleRegister reg_vehicle;

    // O número total de registros em cada arquivo.
    uint32_t n_vehicle_registers = header_vehicle.meta.nroRegistros + header_vehicle.meta.nroRegRemovidos;
    uint32_t n_busline_registers = header_busline.meta.nroRegistros + header_busline.meta.nroRegRemovidos;

    // Contador de registros imprimidos
    int n_matching = 0;

    // Itera por todos os registros de veículo.
    for (int i = 0; i < n_vehicle_registers; i++){
        // Lê e verifica erros na leitura do registro de veículo.
        if (!read_vehicle_register(file_vehicle, &reg_vehicle))
            return handle_error(file_busline, file_vehicle,
                                "could not read register from %s",
                                vehicle_bin_fname);

        // Caso necessário, posiciona o ponteiro de leitura do arquivo de linha
        // logo após o header, ou seja, onde começam os registros.
        if (i > 0)
            fseek(file_busline, BUS_LINE_HEADER_SIZE, SEEK_SET);

        // Se o veículo estiver removido, leia outro veículo até achar um que
        // não esteja.
        if (reg_vehicle.removido == '0') {
            vehicle_drop(reg_vehicle);
            continue;
        }

        // Itera por todos os registros de linha
        for (int j = 0; j < n_busline_registers; j++){
            // Lê e verifica erros na leitura do registro de linha.
            if (!read_bus_line_register(file_busline, &reg_busline))
                return handle_error(file_busline, file_vehicle,
                                    "could not read register from %s",
                                    busline_bin_fname);

            // Se a linha não estiver removida, imprime o veículo seguido da
            // linha caso os códigos sejam iguais.

            bool removed_busline = reg_busline.removido == '0';

            if(reg_vehicle.codLinha == reg_busline.codLinha && !removed_busline){
                print_vehicle(stdout, &reg_vehicle, &header_vehicle);
                print_bus_line(stdout, &reg_busline, &header_busline);
                fprintf(stdout, "\n");

                // Adiciona o contador de registros imprimidos.
                n_matching++; 
            }
            bus_line_drop(reg_busline);
        }
        vehicle_drop(reg_vehicle);
    }

    // Closes the binary files
    fclose(file_busline);
    fclose(file_vehicle);

    // Prints an error message if no match if found
    return checks_matching(n_matching);
}

/**
 * Exibe os resultados que satisfazem a busca de codLinha no arquivo binário de veículos e
 * no arquivo binário de linha de ônibus
 *
 * @param vehicle_bin_fname - caminho para o arquivo binário de veículos
 * @param busline_bin_fname - caminho para o arquivo binário de linhas de ônibus
 * @param index_btree_fname - caminho para o arquivo binário de índices árvore-B
 * @returns - um valor booleano = true se a leitura dos arquivos der certo e retornar algum
 *            resultado, false se a leitura dos arquivos der errado ou não retornar nenhum
 *            resultado da busca
 */
bool join_vehicle_and_bus_line_using_btree(
    const char *vehicle_bin_fname,
    const char *busline_bin_fname,
    const char *index_btree_fname
){
    // Abre os dois arquivos e verifica se foi bem sucedido

    FILE *file_vehicle = NULL;
    FILE *file_busline = NULL;

    file_vehicle = fopen(vehicle_bin_fname, "rb");
    if(!file_vehicle)
        return handle_error(file_busline, file_vehicle,
                            "could not open %s",
                            vehicle_bin_fname);

    file_busline = fopen(busline_bin_fname, "rb");
    if(!file_busline)
        return handle_error(file_busline, file_vehicle,
                            "could not open %s",
                            busline_bin_fname);

    // Lê os headers de ambos os binários e da btree e verifica se houve algum erro.
    BTreeMap btree = btree_new();

    DBVehicleHeader header_vehicle;
    if (!read_header_vehicle(file_vehicle, &header_vehicle))
        return handle_error_btree(file_vehicle, file_busline, btree,
                                  "could not read header from %s",
                                  vehicle_bin_fname);

    DBBusLineHeader header_busline;
    if (!read_header_bus_line(file_busline, &header_busline))
        return handle_error_btree(file_vehicle, file_busline, btree,
                                  "could not read header from %s",
                                  vehicle_bin_fname);

    if (btree_load(&btree, index_btree_fname) != BTREE_OK)
        return handle_error_btree(file_vehicle, file_busline, btree, NULL);

    uint32_t n_vehicle_registers = header_vehicle.meta.nroRegistros + header_vehicle.meta.nroRegRemovidos;

    int n_matching = 0;
    // Loops and reads all the binary vehicle registers
    for (int i = 0; i < n_vehicle_registers; i++){
        // Lê o registro de veículo e verifica se ocorreu erro.
        DBVehicleRegister reg_vehicle;
        if (!read_vehicle_register(file_vehicle, &reg_vehicle))
            return handle_error_btree(file_vehicle, file_busline, btree,
                                      "failed to read register from %s",
                                      vehicle_bin_fname);

        // Verifica se o atual registro veículo está marcado como removido. Se
        // estiver, passa para a próxima iteração.
        if(reg_vehicle.removido == '0') {
            vehicle_drop(reg_vehicle);
            continue;
        }

        int64_t off = btree_get(&btree, reg_vehicle.codLinha);
        if(btree_has_error(&btree)) 
            return handle_error_btree(file_vehicle, file_busline, btree, NULL);

        if(off >= 0){
            fseek(file_busline, off, SEEK_SET);

            // Lê o registro de linha e verifica se ocorreu erro.
            DBBusLineRegister reg_busline;
            if (!read_bus_line_register(file_busline, &reg_busline))
                return handle_error_btree(file_vehicle, file_busline, btree,
                                          "failed to read bus line register from %s",
                                          busline_bin_fname);

            // Imprime ambos os registros
            print_vehicle(stdout, &reg_vehicle, &header_vehicle);
            print_bus_line(stdout, &reg_busline, &header_busline);
            fprintf(stdout, "\n");
            n_matching++;
            bus_line_drop(reg_busline);
        }
        vehicle_drop(reg_vehicle);
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
bool join_vehicle_and_bus_line_merge_sorted(const char *vehicle_bin_fname, const char *busline_bin_fname) {
    // Cria os nomes dos arquivos ordenados como sendo o mesmo nome dos arquivos
    // originais mas com o sufixo "_ordenado".
    char *sorted_vehicle_bin_fname = alloc_sprintf("%s_ordenado", vehicle_bin_fname);
    char *sorted_busline_bin_fname = alloc_sprintf("%s_ordenado", busline_bin_fname);

    // Chama as funcionalidades que ordenam os arquivos de linha e veículo.
    if (!sort_vehicle_bin_file(vehicle_bin_fname, sorted_vehicle_bin_fname) ||
        !sort_bus_line_bin_file(busline_bin_fname, sorted_busline_bin_fname))
    {
        free(sorted_vehicle_bin_fname);
        free(sorted_busline_bin_fname);
        return false;
    }
    // Tenta abrir os dois arquivos gerados pelas funcionalidades de ordenação e
    // libera os nomes que foram alocados dinamicamente.

    FILE *sorted_vehicle_fp = NULL;
    FILE *sorted_busline_fp = NULL;

    sorted_vehicle_fp = fopen(sorted_vehicle_bin_fname, "rb");
    if(!sorted_vehicle_fp) {
        free(sorted_vehicle_bin_fname);
        free(sorted_busline_bin_fname);
        return handle_error(sorted_vehicle_fp, sorted_busline_fp,
                            "could not open %s",
                            vehicle_bin_fname);
    }

    sorted_busline_fp = fopen(sorted_busline_bin_fname, "rb");
    if(!sorted_busline_fp) {
        free(sorted_vehicle_bin_fname);
        free(sorted_busline_bin_fname);
        return handle_error(sorted_vehicle_fp, sorted_busline_fp,
                            "could not open %s",
                            busline_bin_fname);
    }

    free(sorted_vehicle_bin_fname);
    free(sorted_busline_bin_fname);

    // Carrega o header de cada um dos arquivos binários, sempre verificando se
    // ocorreu algum erro.

    DBVehicleHeader header_vehicle;
    if (!read_header_vehicle(sorted_vehicle_fp, &header_vehicle))
        return handle_error(sorted_vehicle_fp, sorted_busline_fp,
                            "could not read header from %s",
                            vehicle_bin_fname);

    DBBusLineHeader header_busline;
    if (!read_header_bus_line(sorted_busline_fp, &header_busline))
        return handle_error(sorted_vehicle_fp, sorted_busline_fp,
                            "could not read header from %s",
                            vehicle_bin_fname);

    DBBusLineRegister reg_busline;
    DBVehicleRegister reg_vehicle;

    // Os arquivos binários ordenados não possuem registros removidos, então o
    // número total de registros é simplesmente `nroRegistros`.

    uint32_t n_vehicle_registers = header_vehicle.meta.nroRegistros;
    uint32_t n_busline_registers = header_busline.meta.nroRegistros;

    uint32_t vehicle_count = 0;
    uint32_t busline_count = 0;

    uint32_t n_matching = 0;

    // Zera os registros para garantir que não tenha lixo de memória. Isso
    // também garante que `reg_vehicle.codLinha` e `reg_busline.codLinha` são 0.
    memset(&reg_vehicle, 0, sizeof(DBVehicleRegister));
    memset(&reg_busline, 0, sizeof(DBBusLineRegister));

    // Itera pelos registros de maneira intercalada. Enquanto os códigos de
    // linha forme iguais, ele continua imprimindo e lendo registros de veículo,
    // quando o código dos registros de veículo ultrapassam, avança os registros
    // de linha até encontrar a correspondente. Isso fornece tempo linear de
    // acordo com a soma da quantidade de registros em ambos os arquivos.

    while (vehicle_count < n_vehicle_registers && busline_count <= n_busline_registers) {
        vehicle_drop(reg_vehicle);
        if (!read_vehicle_register(sorted_vehicle_fp, &reg_vehicle))
            return handle_error(sorted_vehicle_fp, sorted_busline_fp,
                                "could not read vehicle register");

        vehicle_count++;

        while (busline_count < n_busline_registers && reg_busline.codLinha < reg_vehicle.codLinha) {
            bus_line_drop(reg_busline);
            if (!read_bus_line_register(sorted_busline_fp, &reg_busline))
                return handle_error(sorted_vehicle_fp, sorted_busline_fp,
                                    "could not read bus line register");

            busline_count++;
        }

        if (reg_vehicle.codLinha == reg_busline.codLinha) {
            n_matching++;
            print_vehicle(stdout, &reg_vehicle, &header_vehicle);
            print_bus_line(stdout, &reg_busline, &header_busline);
            printf("\n");
        }
    }

    // Libera tudo que foi alocado e retorna.

    vehicle_drop(reg_vehicle);
    bus_line_drop(reg_busline);

    fclose(sorted_vehicle_fp);
    fclose(sorted_busline_fp);

    // Se não houveram registros imprimidos imprime a mensagem correspondente e
    // retorna `false`.
    return checks_matching(n_matching);
}
