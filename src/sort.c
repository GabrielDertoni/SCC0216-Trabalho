#include <stdio.h>
#include <stdlib.h>

#include <common.h>
#include <bin.h>
#include <utils.h>

// Verifica se o arquivo existe no diretório. Se sim, retorna true, se não, exibe a mensagem de erro correspondente e retorna false
static bool check_file(FILE *fp){
    if(!fp){
        printf(ERROR_FOUND);
        return false;
    }
    else{
        return true;
    }
}

/*
* Função auxiliar para comparar os valores do código de veículos para a ordenação
* @param data - endereço de memória do DBVehicle a ser ordenado
* @param i - primeira posição do valor a ser acessada
* @param j - segunda posição do valor a se acessada
* @returns - o resultado da comparação. -1 se o primeiro valor for menor, 0 se os valores forem iguais, 1 se o primeiro valor for maior.
*/
int32_t access_vehicle_register(void *data, int32_t i, int32_t j){
    DBVehicleRegister *vehicle = (DBVehicleRegister*)data;
    if(vehicle[i].codLinha < vehicle[j].codLinha) return -1;
    else if(vehicle[i].codLinha == vehicle[j].codLinha) return 0;
    else return 1;
}

/*
* Função auxiliar para comparar os valores do código da linha de ônibus para a ordenação
* @param data - endereço de memória do DBBusLineRegister a ser ordenado
* @param i - primeira posição do valor a ser acessada
* @param j - segunda posição do valor a se acessada
* @returns - o resultado da comparação. -1 se o primeiro valor for menor, 0 se os valores forem iguais, 1 se o primeiro valor for maior.
*/
int32_t access_busline_register(void *data, int32_t i, int32_t j){
    DBBusLineRegister *bus_line = (DBBusLineRegister*)data;
    if(bus_line[i].codLinha < bus_line[j].codLinha) return -1;
    else if(bus_line[i].codLinha == bus_line[j].codLinha) return 0;
    else return 1;
}

/*
* Lê os dados de um arquivo binário de veículos em RAM, ordena esses dados e os escreve em um novo arquivo binário.
* @param bin_fname - caminho para o arquivo binário de veículos a ser lido
* @param ordered_bin_fname - caminho para o arquivo binário de veículos ordenado a ser escrito
* @returns - um valor booleano = true deu tudo certo, false algum erro ocorreu (uma mensagem de erro será exibida)
*/
bool sort_vehicle_bin_file(const char *bin_fname, const char *ordered_bin_fname){
    FILE *bin_file = fopen(bin_fname, "rb");

    if(!check_file(bin_file)) return false;

    FILE *ordered_file = fopen(ordered_bin_fname, "wb");

    if(!check_file(ordered_file)) {
        fclose(bin_file);
        return false;
    }

    DBVehicleHeader header;
    if (!read_header_vehicle(bin_file, &header)) {
        printf(ERROR_FOUND);
        fclose(bin_file);
        fclose(ordered_file);
        return false;
    }

    // Aloca o espaço de memória para armazenar os dados lidos do arquivo binário na RAM
    DBVehicleRegister *reg = malloc(header.meta.nroRegistros * sizeof(DBVehicleRegister));
    int32_t i = 0;
    // Lê todos os registros do arquivo binário de dados de veículo e apenas armazena na RAM os itens que não estão marcados como removidos
    while (read_vehicle_register(bin_file, &reg[i])){
        if(reg[i].removido != '0') {
            i++;
        } else {
            vehicle_drop(reg[i]);
        }
    }

    fclose(bin_file);

    header.meta.nroRegRemovidos = 0; // Não pode ter itens removidos, logo a quantidade de itens removidos é zero
    header.meta.nroRegistros = i;   // O númerode reigstro é a quantidade de registros lidos, armazenados na variável i
    // Ordena os valores lidos pela lógica do mergesort
    mergesort(reg, sizeof(DBVehicleRegister), 0, i - 1, access_vehicle_register);
    // Pula o espaço do cabeçalho que será escrito depois
    fseek(ordered_file, VEHICLE_HEADER_SIZE, SEEK_SET);

    // Escreve todos dados ordenados no arquivo binário
    for (int j = 0; j < i; j++) {
        if (!write_vehicle_registers(&reg[j], ordered_file)) {
            printf(ERROR_FOUND);
            fclose(ordered_file);
            return false;
        }
        vehicle_drop(reg[j]);
    }

    // Encontra a posição final do arquivo binário ordenado e define com o byteProxReg do cabeçalho
    header.meta.byteProxReg = ftell(ordered_file);

    // Escreve o cabeçalho com as informações certas
    if(!write_vehicles_header(&header, ordered_file)){
        printf(ERROR_FOUND);
        fclose(ordered_file);
        return false;
    }   

    free(reg);
    fclose(ordered_file);
    return true;
}

/*
* Lê os dados de um arquivo binário de linhas de ônibus em RAM, ordena esses dados e os escreve em um novo arquivo binário.
* @param bin_fname - caminho para o arquivo binário de linhas de ônibus a ser lido
* @param ordered_bin_fname - caminho para o arquivo binário de linhas de ônibus ordenado a ser escrito
* @returns - um valor booleano = true deu tudo certo, false algum erro ocorreu (uma mensagem de erro será exibida)
*/
bool sort_bus_line_bin_file(const char *bin_fname, const char *ordered_bin_fname){
    FILE *bin_file = fopen(bin_fname, "rb");

    if(!check_file(bin_file)) return false;

    FILE *ordered_file = fopen(ordered_bin_fname, "wb");

    if(!check_file(ordered_file)) {
        fclose(bin_file);
        return false;
    }

    DBBusLineHeader header;
    if (!read_header_bus_line(bin_file, &header)) {
        printf(ERROR_FOUND);
        fclose(bin_file);
        fclose(ordered_file);
        return false;
    }

    // Aloca o espaço de memória para armazenar os dados lidos do arquivo binário na RAM
    DBBusLineRegister *reg = malloc(header.meta.nroRegistros * sizeof(DBBusLineRegister));
    int32_t i = 0;
    // Lê todos os registros do arquivo binário de dados de veículo e apenas armazena os itens que não estão marcados como removidos
    while (read_bus_line_register(bin_file, &reg[i])){
        if(reg[i].removido != '0') {
            i++;
        } else {
            bus_line_drop(reg[i]);
        }
    }

    fclose(bin_file);

    header.meta.nroRegRemovidos = 0; // Não pode ter itens removidos, logo a quantidade de itens removidos é zero
    header.meta.nroRegistros = i;   // O númerode reigstro é a quantidade de registros lidos, armazenados na variável i
    // Ordena os valores lidos pela lógica do mergesort
    mergesort(reg, sizeof(DBBusLineRegister), 0, i - 1, access_busline_register);

    // Pula o espaço do cabeçalho que será escrito depois
    fseek(ordered_file, BUS_LINE_HEADER_SIZE, SEEK_SET);

    // Escreve todos dados ordenados no arquivo binário
    for(int j = 0; j < i; j++){
        if(!write_bus_line_register(&reg[j], ordered_file)) {
            printf(ERROR_FOUND);
            fclose(ordered_file);
            return false;
        }
        bus_line_drop(reg[j]);
    }

    // Encontra a posição final do arquivo binário ordenado e define com o byteProxReg do cabeçalho
    header.meta.byteProxReg = ftell(ordered_file);

    // Escreve o cabeçalho com as informações certas
    if(!write_bus_lines_header(&header, ordered_file)){
        printf(ERROR_FOUND);
        free(reg);
        fclose(ordered_file);
        return false;
    }   

    free(reg);
    fclose(ordered_file);
    return true;
}
