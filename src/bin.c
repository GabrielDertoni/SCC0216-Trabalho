#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <common.h>
#include <bin.h>
#include<parsing.h>
#include<utils.h>
#include<index.h>

// Macro que verifica se alguma expressão é igual a 1. Se ela não é, retorna
// `false` da função.
#define ASSERT(expr) if ((expr) != 1) return false

static inline void position(FILE *fp, size_t off) {
    if (ftell(fp) != off)
        fseek(fp, off, SEEK_SET);
}

bool update_header_status(char new_val, FILE *fp) {
    position(fp, 0);
    ASSERT(fwrite(&new_val, sizeof(new_val), 1, fp));
    return true;
}

bool update_header_meta(const DBMeta *meta, FILE *fp) {
    position(fp, 0);
    ASSERT(write_header_meta(meta, fp));
    return true;
}

bool write_header_meta(const DBMeta *meta, FILE *fp) {
    ASSERT(fwrite(&meta->status           , sizeof(meta->status)         , 1, fp));
    ASSERT(fwrite(&meta->byteProxReg      , sizeof(meta->byteProxReg)    , 1, fp));
    ASSERT(fwrite(&meta->nroRegistros     , sizeof(meta->nroRegistros)   , 1, fp));
    ASSERT(fwrite(&meta->nroRegRemovidos  , sizeof(meta->nroRegRemovidos), 1, fp));
    return true;
}

bool write_vehicles_header(const DBVehicleHeader *header, FILE *fp) {
    fseek(fp, 0, SEEK_SET);
    ASSERT(write_header_meta(&header->meta, fp));
    ASSERT(fwrite(&header->descrevePrefixo  , sizeof(header->descrevePrefixo)  , 1, fp));
    ASSERT(fwrite(&header->descreveData     , sizeof(header->descreveData)     , 1, fp));
    ASSERT(fwrite(&header->descreveLugares  , sizeof(header->descreveLugares)  , 1, fp));
    ASSERT(fwrite(&header->descreveLinhas   , sizeof(header->descreveLinhas)   , 1, fp));
    ASSERT(fwrite(&header->descreveModelo   , sizeof(header->descreveModelo)   , 1, fp));
    ASSERT(fwrite(&header->descreveCategoria, sizeof(header->descreveCategoria), 1, fp));
    return true;
}

bool write_vehicle(const Vehicle *vehicle, FILE *fp) {
    char prefixo[5];

    char removido;

    if (vehicle->prefixo[0] == REMOVED_MARKER) {
        removido = '0';
        memcpy(prefixo, &vehicle->prefixo[1], sizeof(prefixo) - 1);
        prefixo[4] = '\0';
    } else {
        removido = '1';
        memcpy(prefixo, vehicle->prefixo, sizeof(prefixo));
    }

    uint32_t tamanhoModelo    = vehicle->modelo ? strlen(vehicle->modelo) : 0;
    uint32_t tamanhoCategoria = vehicle->categoria ? strlen(vehicle->categoria) : 0;
    uint32_t tamanhoRegistro  = 0;
    tamanhoRegistro += sizeof(vehicle->prefixo);
    tamanhoRegistro += sizeof(vehicle->data);
    tamanhoRegistro += sizeof(vehicle->quantidadeLugares);
    tamanhoRegistro += sizeof(vehicle->codLinha);
    tamanhoRegistro += sizeof(tamanhoModelo);
    tamanhoRegistro += tamanhoModelo;
    tamanhoRegistro += sizeof(tamanhoCategoria);
    tamanhoRegistro += tamanhoCategoria;

    ASSERT(fwrite(&removido                  , sizeof(removido)                  , 1, fp));
    ASSERT(fwrite(&tamanhoRegistro           , sizeof(tamanhoRegistro)           , 1, fp));
    ASSERT(fwrite(prefixo                    , sizeof(vehicle->prefixo)          , 1, fp));
    ASSERT(fwrite(vehicle->data              , sizeof(vehicle->data)             , 1, fp));
    ASSERT(fwrite(&vehicle->quantidadeLugares, sizeof(vehicle->quantidadeLugares), 1, fp));
    ASSERT(fwrite(&vehicle->codLinha         , sizeof(vehicle->codLinha)         , 1, fp));
    ASSERT(fwrite(&tamanhoModelo             , sizeof(tamanhoModelo)             , 1, fp));

    if (tamanhoModelo > 0) {
        ASSERT(fwrite(vehicle->modelo        , tamanhoModelo * sizeof(char)      , 1, fp));
    }

    ASSERT(fwrite(&tamanhoCategoria          , sizeof(tamanhoCategoria)          , 1, fp));

    if (tamanhoCategoria > 0) {
        ASSERT(fwrite(vehicle->categoria     , tamanhoCategoria * sizeof(char)   , 1, fp));
    }

    return true;
}

/*
* Escreve os dados de DBVehicleRegister em um arquivo binário
* @param line - struct do tipo DBVehicleRegister
* @param fp - ponteiro para o arquivo binário
* @returns - um valor booleano = true se a escrita deu certo, false se deu errado.
*/
bool write_vehicle_registers(const DBVehicleRegister *vehicle, FILE *fp) {
    char prefixo[5];

    char removido;

    if (vehicle->prefixo[0] == REMOVED_MARKER) {
        removido = '0';
        memcpy(prefixo, &vehicle->prefixo[1], sizeof(prefixo) - 1);
        prefixo[4] = '\0';
    } else {
        removido = '1';
        memcpy(prefixo, vehicle->prefixo, sizeof(prefixo));
    }

    uint32_t tamanhoModelo    = vehicle->modelo ? strlen(vehicle->modelo) : 0;
    uint32_t tamanhoCategoria = vehicle->categoria ? strlen(vehicle->categoria) : 0;
    uint32_t tamanhoRegistro  = 0;
    tamanhoRegistro += sizeof(vehicle->prefixo);
    tamanhoRegistro += sizeof(vehicle->data);
    tamanhoRegistro += sizeof(vehicle->quantidadeLugares);
    tamanhoRegistro += sizeof(vehicle->codLinha);
    tamanhoRegistro += sizeof(tamanhoModelo);
    tamanhoRegistro += tamanhoModelo;
    tamanhoRegistro += sizeof(tamanhoCategoria);
    tamanhoRegistro += tamanhoCategoria;

    ASSERT(fwrite(&removido                  , sizeof(removido)                  , 1, fp));
    ASSERT(fwrite(&tamanhoRegistro           , sizeof(tamanhoRegistro)           , 1, fp));
    ASSERT(fwrite(prefixo                    , sizeof(vehicle->prefixo)          , 1, fp));
    ASSERT(fwrite(vehicle->data              , sizeof(vehicle->data)             , 1, fp));
    ASSERT(fwrite(&vehicle->quantidadeLugares, sizeof(vehicle->quantidadeLugares), 1, fp));
    ASSERT(fwrite(&vehicle->codLinha         , sizeof(vehicle->codLinha)         , 1, fp));
    ASSERT(fwrite(&tamanhoModelo             , sizeof(tamanhoModelo)             , 1, fp));

    if (tamanhoModelo > 0) {
        ASSERT(fwrite(vehicle->modelo        , tamanhoModelo * sizeof(char)      , 1, fp));
    }

    ASSERT(fwrite(&tamanhoCategoria          , sizeof(tamanhoCategoria)          , 1, fp));

    if (tamanhoCategoria > 0) {
        ASSERT(fwrite(vehicle->categoria     , tamanhoCategoria * sizeof(char)   , 1, fp));
    }

    return true;
}

bool write_bus_lines_header(const DBBusLineHeader *header, FILE *fp) {
    fseek(fp, 0, SEEK_SET);
    ASSERT(write_header_meta(&header->meta, fp));
    ASSERT(fwrite(&header->descreveCodigo, sizeof(header->descreveCodigo), 1, fp));
    ASSERT(fwrite(&header->descreveCartao, sizeof(header->descreveCartao), 1, fp));
    ASSERT(fwrite(&header->descreveNome  , sizeof(header->descreveNome  ), 1, fp));
    ASSERT(fwrite(&header->descreveCor   , sizeof(header->descreveCor   ), 1, fp));
    return true;
}

bool write_bus_line(const BusLine *line, FILE *fp) {
    int32_t codLinha;

    char removido;
    if (line->codLinha[0] == REMOVED_MARKER) {
        removido = '0';
        codLinha = (int)strtol(&line->codLinha[1], NULL, 10);
    } else {
        removido = '1';
        codLinha = (int)strtol(line->codLinha, NULL, 10);
    }

    uint32_t tamanhoNome = line->nomeLinha ? strlen(line->nomeLinha) : 0;
    uint32_t tamanhoCor  = line->corLinha ? strlen(line->corLinha) : 0;
    uint32_t tamanhoRegistro = 0;
    tamanhoRegistro += sizeof(codLinha);
    tamanhoRegistro += sizeof(line->aceitaCartao);
    tamanhoRegistro += sizeof(tamanhoNome);
    tamanhoRegistro += tamanhoNome;
    tamanhoRegistro += sizeof(tamanhoCor);
    tamanhoRegistro += tamanhoCor;

    ASSERT(fwrite(&removido          , sizeof(removido)          , 1, fp));
    ASSERT(fwrite(&tamanhoRegistro   , sizeof(tamanhoRegistro)   , 1, fp));
    ASSERT(fwrite(&codLinha          , sizeof(codLinha)          , 1, fp));
    ASSERT(fwrite(&line->aceitaCartao, sizeof(line->aceitaCartao), 1, fp));
    ASSERT(fwrite(&tamanhoNome       , sizeof(tamanhoNome)       , 1, fp));

    if (tamanhoNome > 0) {
        ASSERT(fwrite(line->nomeLinha, tamanhoNome * sizeof(char), 1, fp));
    }

    ASSERT(fwrite(&tamanhoCor        , sizeof(tamanhoCor)        , 1, fp));

    if (tamanhoCor > 0) {
        ASSERT(fwrite(line->corLinha , tamanhoCor * sizeof(char) , 1, fp));
    }
    return true;
}

/*
* Escreve os dados de DBBusLineRegister em um arquivo binário
* @param line - struct do tipo DBBusLineRegister
* @param fp - ponteiro para o arquivo binário
* @returns - um valor booleano = true se a escrita deu certo, false se deu errado.
*/
bool write_bus_line_register(const DBBusLineRegister *line, FILE *fp) {
    int32_t codLinha;

    char removido;
    if (line->codLinha == REMOVED_MARKER) {
        removido = '0';
        codLinha = line->codLinha;
    } else {
        removido = '1';
        codLinha = line->codLinha;
    }

    uint32_t tamanhoNome = line->nomeLinha ? strlen(line->nomeLinha) : 0;
    uint32_t tamanhoCor  = line->corLinha ? strlen(line->corLinha) : 0;
    uint32_t tamanhoRegistro = 0;
    tamanhoRegistro += sizeof(codLinha);
    tamanhoRegistro += sizeof(line->aceitaCartao);
    tamanhoRegistro += sizeof(tamanhoNome);
    tamanhoRegistro += tamanhoNome;
    tamanhoRegistro += sizeof(tamanhoCor);
    tamanhoRegistro += tamanhoCor;

    ASSERT(fwrite(&removido          , sizeof(removido)          , 1, fp));
    ASSERT(fwrite(&tamanhoRegistro   , sizeof(tamanhoRegistro)   , 1, fp));
    ASSERT(fwrite(&codLinha          , sizeof(codLinha)          , 1, fp));
    ASSERT(fwrite(&line->aceitaCartao, sizeof(line->aceitaCartao), 1, fp));
    ASSERT(fwrite(&tamanhoNome       , sizeof(tamanhoNome)       , 1, fp));

    if (tamanhoNome > 0) {
        ASSERT(fwrite(line->nomeLinha, tamanhoNome * sizeof(char), 1, fp));
    }

    ASSERT(fwrite(&tamanhoCor        , sizeof(tamanhoCor)        , 1, fp));

    if (tamanhoCor > 0) {
        ASSERT(fwrite(line->corLinha , tamanhoCor * sizeof(char) , 1, fp));
    }
    return true;
}


// Lê os metadados dos arquivos binários
bool read_meta(FILE *fp, DBMeta *meta){
    ASSERT(fread(&meta->status, 1, 1, fp));
    ASSERT(meta->status == '1');
    ASSERT(fread(&meta->byteProxReg, sizeof(long), 1, fp));
    ASSERT(fread(&meta->nroRegistros, sizeof(int), 1, fp));
    ASSERT(fread(&meta->nroRegRemovidos, sizeof(int), 1, fp));
    return true;
}

// Lê o cabeçalho de um arquivo binário que contém os registros de veículo
bool read_header_vehicle(FILE *fp, DBVehicleHeader *header){
    ASSERT(read_meta(fp, &header->meta));
    ASSERT(fread(&header->descrevePrefixo, 18, 1, fp));
    ASSERT(fread(&header->descreveData, 35, 1, fp));
    ASSERT(fread(&header->descreveLugares, 42, 1, fp));
    ASSERT(fread(&header->descreveLinhas, 26, 1, fp));
    ASSERT(fread(&header->descreveModelo, 17, 1, fp));
    ASSERT(fread(&header->descreveCategoria, 20, 1, fp));
    return true;
}

// Lê o cabeçalho de um arquivo binário que contém os registros das linhas de ônibus
bool read_header_bus_line(FILE *fp, DBBusLineHeader *header){
    ASSERT(read_meta(fp, &header->meta));
    ASSERT(fread(&header->descreveCodigo, 15, 1, fp));
    ASSERT(fread(&header->descreveCartao, 13, 1, fp));
    ASSERT(fread(&header->descreveNome, 13, 1, fp));
    ASSERT(fread(&header->descreveCor, 24, 1, fp));
    return true;
}

// Imprime a data de entrada de um veículo na frota no formato 'DD de texto(MM) de AAAA'
static void print_date(const char (*date)[10], FILE *out, const char (*print)[35]) {
    const char *months[12] = { "janeiro", "fevereiro", "março", "abril", "maio",
                               "junho", "julho", "agosto", "setembro", "outubro",
                               "novembro", "dezembro" };

    // Copia `date` para garantir `const` ao parâmetro ao usar `strsep`.
    char date_cpy[10];
    memcpy(date_cpy, *date, sizeof(*date));

    char *parse_ptr = date_cpy;
    char *year = strsep(&parse_ptr, "-");
    char *month = strsep(&parse_ptr, "-");
    char *day = strsep(&parse_ptr, "-");
    fprintf(out, "%.35s: %.2s de %s de %s\n", *print, day, months[atoi(month)-1], year);
}

// Imprime as informações de busca do arquivo binário de veículo
void print_vehicle(FILE *out, const DBVehicleRegister *reg, const DBVehicleHeader *header){
    fprintf(out, "%.18s: %.5s\n", header->descrevePrefixo, reg->prefixo);

    if(reg->tamanhoModelo != 0)
        fprintf(out, "%.17s: %s\n", header->descreveModelo, reg->modelo);
    else
        fprintf(out, "%.17s: %s\n", header->descreveModelo, NO_VALUE);

    if(reg->tamanhoCategoria != 0)
        fprintf(out, "%.20s: %s\n", header->descreveCategoria, reg->categoria);
    else
        fprintf(out, "%.20s: %s\n", header->descreveCategoria, NO_VALUE);

    if(strlen(reg->data) != 0)
        print_date(&reg->data, out, &header->descreveData);
    else
        fprintf(out, "%.35s: %s\n", header->descreveData, NO_VALUE);

    if(reg->quantidadeLugares != -1)
        fprintf(out, "%.42s: %d\n", header->descreveLugares, reg->quantidadeLugares);
    else
        fprintf(out, "%.42s: %s\n", header->descreveLugares, NO_VALUE);
}

// Imprime as informações de busca do arquivo binário das linhas de ônibus
void print_bus_line(FILE *out, const DBBusLineRegister *reg, const DBBusLineHeader *header){
    fprintf(out, "%.15s: %d\n", header->descreveCodigo, reg->codLinha);
    if(reg->tamanhoNome != 0)
        fprintf(out, "%.13s: %s\n", header->descreveNome, reg->nomeLinha);
    else
        fprintf(out, "%.13s: %s\n", header->descreveNome, NO_VALUE);

    if(reg->tamanhoCor != 0)
        fprintf(out, "%.24s: %s\n", header->descreveCor, reg->corLinha);
    else
        fprintf(out, "%.24s: %s\n", header->descreveCor, NO_VALUE);

    switch(reg->aceitaCartao){
        case 'S':
            fprintf(out, "%.13s: %s\n", header->descreveCartao, YES);
            break;
        case 'N':
            fprintf(out, "%.13s: %s\n", header->descreveCartao, NO);
            break;
        case 'F':
            fprintf(out, "%.13s: %s\n", header->descreveCartao, WEEKEND);
            break;
        default:
            fprintf(out, "%.13s: %s\n", header->descreveCartao, NO_VALUE);
            break;
    }
}

// Desaloca a memória alocada para as strings categoria e modelo dos veículos
void vehicle_drop(DBVehicleRegister v){
    if (v.categoria) free(v.categoria);
    if (v.modelo) free(v.modelo);
}

// Desaloca a memória alocada para as strings nomeLinha e corLinha das linhas de ônibus
void bus_line_drop(DBBusLineRegister b){
    if (b.nomeLinha) free(b.nomeLinha);
    if (b.corLinha) free(b.corLinha);
}

/*
 * Lê os registros de um arquivo binário de veículos
 * @param fp - ponteiro do arquivo binário
 * @param reg - ponteiro de DBVehicleRegister
 * @return 'true' se for lido com sucesso 'false' se houver algum erro
*/ 
bool read_vehicle_register(FILE *fp, DBVehicleRegister *reg) {
    ASSERT(fread(&reg->removido, 1, 1, fp));
    ASSERT(fread(&reg->tamanhoRegistro, 4, 1, fp));
    ASSERT(fread(&reg->prefixo, 5, 1, fp));
    ASSERT(fread(&reg->data, 10, 1, fp));
    ASSERT(fread(&reg->quantidadeLugares, 4, 1, fp));
    ASSERT(fread(&reg->codLinha, 4, 1, fp));
    ASSERT(fread(&reg->tamanhoModelo, 4, 1, fp));

    reg->modelo = NULL;

    if (reg->tamanhoModelo > 0) {
        reg->modelo = (char *)malloc((reg->tamanhoModelo + 1) * sizeof(char));
        ASSERT(fread(reg->modelo, reg->tamanhoModelo, 1, fp));
        reg->modelo[reg->tamanhoModelo] = '\0';
    }

    ASSERT(fread(&reg->tamanhoCategoria, 4, 1, fp));

    reg->categoria = NULL;

    if (reg->tamanhoCategoria > 0) {
        reg->categoria = (char *)malloc((reg->tamanhoCategoria + 1) * sizeof(char));
        ASSERT(fread(reg->categoria, reg->tamanhoCategoria, 1, fp));
        reg->categoria[reg->tamanhoCategoria] = '\0';
    }

    return true;
}

/*
 * Lê os registros de um arquivo binário de linhas de ônibus
 * @param fp - ponteiro do arquivo binário
 * @param reg - ponteiro de DBBusLineRegister
 * @return 'true' se for lido com sucesso 'false' se houver algum erro
*/
bool read_bus_line_register(FILE *fp, DBBusLineRegister *reg){
    ASSERT(fread(&reg->removido, 1, 1, fp));
    ASSERT(fread(&reg->tamanhoRegistro, 4, 1, fp));
    ASSERT(fread(&reg->codLinha, 4, 1, fp));
    ASSERT(fread(&reg->aceitaCartao, 1, 1, fp));
    ASSERT(fread(&reg->tamanhoNome, 4, 1, fp));

    reg->nomeLinha = NULL;

    if (reg->tamanhoNome > 0) {
        reg->nomeLinha = (char *)malloc((reg->tamanhoNome + 1) * sizeof(char));
        ASSERT(fread(reg->nomeLinha, reg->tamanhoNome, 1, fp));
        reg->nomeLinha[reg->tamanhoNome] = '\0';
    }

    ASSERT(fread(&reg->tamanhoCor, 4, 1, fp));

    reg->corLinha = NULL;

    if (reg->tamanhoCor > 0) {
        reg->corLinha = malloc(reg->tamanhoCor + 1);
        ASSERT(fread(reg->corLinha, reg->tamanhoCor, 1, fp));
        reg->corLinha[reg->tamanhoCor] = '\0';
    }

    return true;
}

/*
 * Verifica se o atual registro satisfaz as condições de busca
 * @param reg - ponteiro que contém as informações do registro de veículos
 * @param field - string que contém o campo a ser buscado
 * @param equals - string que contém o valor a ser comparado
 * @return 'true' se o valor do campo do registro buscado equivale ao valor de busca 'false' se não equivaler
 * 
 * Exibe uma mensagem de erro caso algo inesperado ocorra e termina o programa
*/
bool check_vehicle_field_equals(const DBVehicleRegister *reg, const char *field, const char *equals){
    if(strcmp(field, "prefixo") == 0)
        return strstr(reg->prefixo, equals) != NULL;
    else if(strcmp(field, "data") == 0)
        return strcmp(equals, reg->data) == 0;
    else if(strcmp(field, "quantidadeLugares") == 0)
        return reg->quantidadeLugares == (int)strtol(equals, NULL, 10);
    else if(strcmp(field, "modelo") == 0)
        return strcmp(equals, reg->modelo) == 0;
    else if(strcmp(field, "categoria") == 0)
        return strcmp(equals, reg->categoria) == 0;

    // Nunca deveria acontecer
    fprintf(stderr, "Erro: Invalid field.");
    exit(0);
}

/*
 * Verifica se o atual registro satisfaz as condições de busca
 * @param reg - ponteiro que contém as informações do registro de veículos
 * @param field - string que contém o campo a ser buscado
 * @param equals - string que contém o valor a ser comparado
 * @return 'true' se o valor do campo do registro buscado equivale ao valor de busca 'false' se não equivaler
 * 
 * Exibe uma mensagem de erro caso algo inesperado ocorra e termina o programa
*/
bool check_bus_line_field_equals(const DBBusLineRegister *reg, const char *field, const char *equals){
    if(strcmp(field, "codLinha") == 0)
        return reg->codLinha == (int)strtol(equals, NULL, 10);
    else if(strcmp(field, "aceitaCartao") == 0)
        return *equals == reg->aceitaCartao;
    else if(strcmp(field, "nomeLinha") == 0)
        return strcmp(equals, reg->nomeLinha) == 0;
    else if(strcmp(field, "corLinha") == 0)
        return strcmp(equals, reg->corLinha) == 0;

    // Nunca deveria acontecer
    fprintf(stderr, "Erro: Invalid field.");
    exit(0);
}

// Verifica se o arquivo existe no diretório. Se sim, retorna true, se não, exibe a mensagem de erro correspondente e retorna false
bool check_file(FILE *fp){
    if(!fp){
        printf(ERROR_FOUND);
        return false;
    }
    else{
        return true;
    }
}

bool select_from_vehicle_where(const char *from_file, const char *where_field, const char *equals_to){
    FILE *fp = fopen(from_file, "rb");

    if (!check_file(fp)) return false;

    DBVehicleHeader header;
    if (!read_header_vehicle(fp, &header)) {
        printf(ERROR_FOUND);
        fclose(fp);
        return false;
    }

    bool print = (where_field == NULL);
    bool is_unique_field = where_field && strcmp("prefixo", where_field) == 0;
    DBVehicleRegister reg;

    int n_matching = 0;
    while (read_vehicle_register(fp, &reg)){
        if(where_field != NULL && equals_to != NULL)
            print = reg.removido == '1' && check_vehicle_field_equals(&reg, where_field, equals_to);

        if(print) {
            print_vehicle(stdout, &reg, &header);
            fprintf(stdout, "\n");
            n_matching++;
        }

        vehicle_drop(reg);

        if (is_unique_field && print) break;
    }
    fclose(fp);

    if (n_matching == 0) {
        printf(NO_REGISTER);
        return false;
    }

    return true;
}

bool select_from_bus_line_where(const char *from_file, const char *where_field, const char *equals_to){
    FILE *fp = fopen(from_file, "rb");

    if (!check_file(fp)) return false;

    DBBusLineHeader header;
    if (!read_header_bus_line(fp, &header)) {
        printf(ERROR_FOUND);
        fclose(fp);
        return false;
    }

    bool print = (where_field == NULL);
    bool is_unique_field = where_field && strcmp("codLinha", where_field) == 0;
    DBBusLineRegister reg;

    int n_matching = 0;
    while (read_bus_line_register(fp, &reg)){
        if (where_field != NULL && equals_to != NULL)
            print = reg.removido == '1' && check_bus_line_field_equals(&reg, where_field, equals_to);

        if (print) {
            print_bus_line(stdout, &reg, &header);
            fprintf(stdout, "\n");
            n_matching++;
        }

        bus_line_drop(reg);

        if (is_unique_field && print) break;
    }
    fclose(fp);

    if (n_matching == 0) {
        printf(NO_REGISTER);
        return false;
    }

    return true;
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
bool order_vehicle_bin_file(const char *bin_fname, const char *ordered_bin_fname){
    FILE *bin_file = fopen(bin_fname, "rb");
    FILE *ordered_file = fopen(ordered_bin_fname, "wb");

    if(!check_file(bin_file)) return false;
    if(!check_file(ordered_file)) return false;

    DBVehicleHeader header;
    if (!read_header_vehicle(bin_file, &header)) {
        printf(ERROR_FOUND);
        fclose(bin_file);
        return false;
    }

    // Aloca o espaço de memória para armazenar os dados lidos do arquivo binário na RAM
    DBVehicleRegister *reg = malloc(header.meta.nroRegistros * sizeof(DBVehicleRegister));
    int32_t i = 0;
    // Lê todos os registros do arquivo binário de dados de veículo e apenas armazena na RAM os itens que não estão marcados como removidos
    while (read_vehicle_register(bin_file, &reg[i])){
        if(reg[i].removido != '0')
            i++;
    }

    header.meta.nroRegRemovidos = 0; // Não pode ter itens removidos, logo a quantidade de itens removidos é zero
    header.meta.nroRegistros = i;   // O númerode reigstro é a quantidade de registros lidos, armazenados na variável i
    // Ordena os valores lidos pela lógica do mergesort
    mergesort(reg, sizeof(DBVehicleRegister), 0, i-1, access_vehicle_register);
    // Pula o espaço do cabeçalho que será escrito depois
    fseek(ordered_file, VEHICLE_HEADER_SIZE, SEEK_SET);

    // Escreve todos dados ordenados no arquivo binário
    for(int j = 0; j < i; j++){
        if(!write_vehicle_registers(&reg[j], ordered_file)) return false;
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
    fclose(bin_file);
    fclose(ordered_file);
    return true;
}

/*
* Lê os dados de um arquivo binário de linhas de ônibus em RAM, ordena esses dados e os escreve em um novo arquivo binário.
* @param bin_fname - caminho para o arquivo binário de linhas de ônibus a ser lido
* @param ordered_bin_fname - caminho para o arquivo binário de linhas de ônibus ordenado a ser escrito
* @returns - um valor booleano = true deu tudo certo, false algum erro ocorreu (uma mensagem de erro será exibida)
*/
bool order_bus_line_bin_file(const char *bin_fname, const char *ordered_bin_fname){
    FILE *bin_file = fopen(bin_fname, "rb");
    FILE *ordered_file = fopen(ordered_bin_fname, "wb");

    if(!check_file(bin_file)) return false;
    if(!check_file(ordered_file)) return false;

    DBBusLineHeader header;
    if (!read_header_bus_line(bin_file, &header)) {
        printf(ERROR_FOUND);
        fclose(bin_file);
        return false;
    }

    // Aloca o espaço de memória para armazenar os dados lidos do arquivo binário na RAM
    DBBusLineRegister *reg = malloc(header.meta.nroRegistros * sizeof(DBBusLineRegister));
    int32_t i = 0;
    // Lê todos os registros do arquivo binário de dados de veículo e apenas armazena os itens que não estão marcados como removidos
    while (read_bus_line_register(bin_file, &reg[i])){
        if(reg[i].removido != '0')
            i++;
    }

    header.meta.nroRegRemovidos = 0; // Não pode ter itens removidos, logo a quantidade de itens removidos é zero
    header.meta.nroRegistros = i;   // O númerode reigstro é a quantidade de registros lidos, armazenados na variável i
    // Ordena os valores lidos pela lógica do mergesort
    mergesort(reg, sizeof(DBBusLineRegister), 0, i-1, access_busline_register);

    // Pula o espaço do cabeçalho que será escrito depois
    fseek(ordered_file, BUS_LINE_HEADER_SIZE, SEEK_SET);

    // Escreve todos dados ordenados no arquivo binário
    for(int j = 0; j < i; j++){
        if(!write_bus_line_register(&reg[j], ordered_file)) return false;
    }

    // Encontra a posição final do arquivo binário ordenado e define com o byteProxReg do cabeçalho
    header.meta.byteProxReg = ftell(ordered_file);

    // Escreve o cabeçalho com as informações certas
    if(!write_bus_lines_header(&header, ordered_file)){
        printf(ERROR_FOUND);
        fclose(ordered_file);
        return false;
    }   

    free(reg);
    fclose(bin_file);
    fclose(ordered_file);
    return true;
}

bool merge_sorted(const char *vehicle_bin_fname, const char *busline_bin_fname) {
    char *sorted_vehicle_bin_fname = alloc_sprintf("%s_ordenado", vehicle_bin_fname);
    char *sorted_busline_bin_fname = alloc_sprintf("%s_ordenado", busline_bin_fname);

    if (!order_vehicle_bin_file(vehicle_bin_fname, sorted_vehicle_bin_fname) ||
        !order_bus_line_bin_file(busline_bin_fname, sorted_busline_bin_fname))
    {
        return false;
    }

    FILE *sorted_vehicle_fp = fopen(sorted_vehicle_bin_fname, "rb");

    if (!check_file(sorted_vehicle_fp)) {
        return false;
    }

    FILE *sorted_busline_fp = fopen(sorted_busline_bin_fname, "rb");

    if (!check_file(sorted_busline_fp)) {
        fclose(sorted_vehicle_fp);
        return false;
    }

    DBVehicleHeader header_vehicle;
    if (!read_header_vehicle(sorted_vehicle_fp, &header_vehicle)) {
        printf(ERROR_FOUND);
        fclose(sorted_vehicle_fp);
        fclose(sorted_busline_fp);
        return false;
    }

    // Reads the header of the binary bus line file
    DBBusLineHeader header_busline;
    if (!read_header_bus_line(sorted_busline_fp, &header_busline)) {
        printf(ERROR_FOUND);
        fclose(sorted_vehicle_fp);
        fclose(sorted_busline_fp);
        return false;
    }

    DBBusLineRegister reg_busline;
    DBVehicleRegister reg_vehicle;

    uint32_t n_vehicle_registers = header_vehicle.meta.nroRegistros;
    uint32_t n_busline_registers = header_busline.meta.nroRegistros;

    if (n_vehicle_registers == 0 || n_busline_registers == 0) {
        fclose(sorted_vehicle_fp);
        fclose(sorted_busline_fp);
        return true;
    }

    if (!read_vehicle_register(sorted_vehicle_fp, &reg_vehicle)) {
        printf(ERROR_FOUND);
        fclose(sorted_vehicle_fp);
        fclose(sorted_busline_fp);
        return false;
    }

    if (!read_bus_line_register(sorted_busline_fp, &reg_busline)) {
        vehicle_drop(reg_vehicle);
        printf(ERROR_FOUND);
        fclose(sorted_vehicle_fp);
        fclose(sorted_busline_fp);
        return false;
    }

    uint32_t vehicle_count = 1;
    uint32_t busline_count = 1;

    uint32_t n_matching = 0;

    while (vehicle_count < n_vehicle_registers && busline_count <= n_busline_registers) {
        if (reg_vehicle.codLinha < reg_busline.codLinha) {
            vehicle_drop(reg_vehicle);
            if (!read_vehicle_register(sorted_vehicle_fp, &reg_vehicle)) {
                printf("expected %d vehicle registers but got %d\n", n_vehicle_registers, vehicle_count);
                printf(ERROR_FOUND);
                bus_line_drop(reg_busline);
                fclose(sorted_vehicle_fp);
                fclose(sorted_busline_fp);
                return false;
            }
            vehicle_count++;

        } else if (reg_vehicle.codLinha > reg_busline.codLinha) {
            if (busline_count >= n_busline_registers) break;

            bus_line_drop(reg_busline);
            if (!read_bus_line_register(sorted_busline_fp, &reg_busline)) {
                printf("expected %d busline registers but got %d\n", n_busline_registers, busline_count);
                printf(ERROR_FOUND);
                vehicle_drop(reg_vehicle);
                fclose(sorted_vehicle_fp);
                fclose(sorted_busline_fp);
                return false;
            }
            busline_count++;

        } else {
            n_matching++;
            print_vehicle(stdout, &reg_vehicle, &header_vehicle);
            print_bus_line(stdout, &reg_busline, &header_busline);
            printf("\n");

            if (vehicle_count >= n_vehicle_registers && busline_count >= n_busline_registers) {
                break;
            }

            vehicle_drop(reg_vehicle);

            if (!read_vehicle_register(sorted_vehicle_fp, &reg_vehicle)) {
                bus_line_drop(reg_busline);
                fclose(sorted_vehicle_fp);
                fclose(sorted_busline_fp);
                return false;
            }
            vehicle_count++;
        }
    }

    if (n_matching == 0) {
        printf(NO_REGISTER);
    }

    vehicle_drop(reg_vehicle);
    bus_line_drop(reg_busline);

    fclose(sorted_vehicle_fp);
    fclose(sorted_busline_fp);
    return true;
}
