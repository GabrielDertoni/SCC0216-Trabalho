#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <common.h>
#include <bin.h>
#include<parsing.h>

// Macro que verifica se alguma expressão é igual a 1. Se ela não é, retorna
// `false` da função.
#define ASSERT(expr) if ((expr) != 1) return false

// Macros com mensagens a serem exibidas.
#define NO_REGISTER     "Registro inexistente.\n"
#define NO_VALUE        "campo com valor nulo"
#define YES             "PAGAMENTO SOMENTE COM CARTAO SEM PRESENCA DE COBRADOR"
#define NO              "PAGAMENTO EM CARTAO E DINHEIRO"
#define WEEKEND         "PAGAMENTO EM CARTAO SOMENTE NO FINAL DE SEMANA"

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

bool write_bus_lines_header(const DBBusLineHeader *header, FILE *fp) {
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
        fprintf(out, "%.42s: %d\n\n", header->descreveLugares, reg->quantidadeLugares);
    else
        fprintf(out, "%.42s: %s\n\n", header->descreveLugares, NO_VALUE);
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
            fprintf(out, "%.13s: %s\n\n", header->descreveCartao, YES);
            break;
        case 'N':
            fprintf(out, "%.13s: %s\n\n", header->descreveCartao, NO);
            break;
        case 'F':
            fprintf(out, "%.13s: %s\n\n", header->descreveCartao, WEEKEND);
            break;
        default:
            fprintf(out, "%.13s: %s\n\n", header->descreveCartao, NO_VALUE);
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
