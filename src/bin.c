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

#define NO_REGISTER     "Registro inexistente.\n"
#define NO_VALUE        "campo com valor nulo"
#define PREFIX          "Prefixo do veiculo:"
#define MODEL           "Modelo do veiculo:"
#define CATEGORY        "Categoria do veiculo:"
#define DATE            "Data de entrada do veiculo na frota:"
#define PLACES          "Quantidade de lugares sentados disponiveis:"
#define COD_LINHA       "Codigo da linha:"
#define NAME_LINE       "Nome da linha:"
#define DESCRIBE_COLOR  "Cor que descreve a linha:"
#define ACCEPT          "Aceita cartao:"
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

bool update_header_meta(DBMeta meta, FILE *fp) {
    position(fp, 0);
    ASSERT(write_header_meta(meta, fp));
    return true;
}

bool write_header_meta(DBMeta meta, FILE *fp) {
    ASSERT(fwrite(&meta.status           , sizeof(meta.status)         , 1, fp));
    ASSERT(fwrite(&meta.byteProxReg      , sizeof(meta.byteProxReg)    , 1, fp));
    ASSERT(fwrite(&meta.nroRegistros     , sizeof(meta.nroRegistros)   , 1, fp));
    ASSERT(fwrite(&meta.nroRegRemovidos  , sizeof(meta.nroRegRemovidos), 1, fp));
    return true;
}

bool write_vehicles_header(DBVehicleHeader header, FILE *fp) {
    ASSERT(write_header_meta(header.meta, fp));
    ASSERT(fwrite(&header.descrevePrefixo  , sizeof(header.descrevePrefixo)  , 1, fp));
    ASSERT(fwrite(&header.descreveData     , sizeof(header.descreveData)     , 1, fp));
    ASSERT(fwrite(&header.descreveLugares  , sizeof(header.descreveLugares)  , 1, fp));
    ASSERT(fwrite(&header.descreveLinhas   , sizeof(header.descreveLinhas)   , 1, fp));
    ASSERT(fwrite(&header.descreveModelo   , sizeof(header.descreveModelo)   , 1, fp));
    ASSERT(fwrite(&header.descreveCategoria, sizeof(header.descreveCategoria), 1, fp));
    return true;
}

bool write_vehicle(const Vehicle *vehicle, FILE *fp) {
    char prefixo[5];

    char removido;
    // 
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

bool write_bus_lines_header(DBBusLineHeader header, FILE *fp) {
    ASSERT(write_header_meta(header.meta, fp));
    ASSERT(fwrite(&header.descreveCodigo, sizeof(header.descreveCodigo), 1, fp));
    ASSERT(fwrite(&header.descreveCartao, sizeof(header.descreveCartao), 1, fp));
    ASSERT(fwrite(&header.descreveNome  , sizeof(header.descreveNome  ), 1, fp));
    ASSERT(fwrite(&header.descreveCor   , sizeof(header.descreveCor   ), 1, fp));
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

bool read_meta(FILE *fp, DBMeta *meta){
    ASSERT(fread(&meta->status, 1, 1, fp));
    ASSERT(meta->status == '1');
    ASSERT(fread(&meta->byteProxReg, sizeof(long), 1, fp));
    ASSERT(fread(&meta->nroRegistros, sizeof(int), 1, fp));
    ASSERT(fread(&meta->nroRegRemovidos, sizeof(int), 1, fp));
    return true;
}

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

bool read_header_bus_line(FILE *fp, DBBusLineHeader *header){
    ASSERT(read_meta(fp, &header->meta));
    ASSERT(fread(&header->descreveCodigo, 15, 1, fp));
    ASSERT(fread(&header->descreveCartao, 13, 1, fp));
    ASSERT(fread(&header->descreveNome, 13, 1, fp));
    ASSERT(fread(&header->descreveCor, 24, 1, fp));
    return true;
}

static void print_date(char date[10], FILE *out) {
    const char *months[12] = { "janeiro", "fevereiro", "março", "abril", "maio",
                               "junho", "julho", "agosto", "setembro", "outubro",
                               "novembro", "dezembro" };
    char *parse_ptr = date;
    char *year = strsep(&parse_ptr, "-");
    char *month = strsep(&parse_ptr, "-");
    char *day = strsep(&parse_ptr, "-");
    fprintf(out, "%s %.2s de %s de %s\n", DATE, day, months[atoi(month)-1], year);
}

void print_vehicle(FILE *out, DBVehicleRegister *reg){
    fprintf(out, "%s %.5s\n", PREFIX, reg->prefixo);

    if(reg->tamanhoModelo != 0)
        fprintf(out, "%s %s\n", MODEL, reg->modelo);
    else
        fprintf(out, "%s %s\n", MODEL, NO_VALUE);

    if(reg->tamanhoCategoria != 0)
        fprintf(out, "%s %s\n", CATEGORY, reg->categoria);
    else
        fprintf(out, "%s %s\n", CATEGORY, NO_VALUE);

    if(strlen(reg->data) != 0)
        print_date(reg->data, out);
    else
        fprintf(out, "%s %s\n", DATE, NO_VALUE);

    if(reg->quantidadeLugares != -1)
        fprintf(out, "%s %d\n\n", PLACES, reg->quantidadeLugares);
    else
        fprintf(out, "%s %s\n\n", PLACES, NO_VALUE);
}

void print_bus_line(FILE *out, DBBusLineRegister *reg){
    fprintf(out, "%s %d\n", COD_LINHA, reg->codLinha);
    if(reg->tamanhoNome != 0)
        fprintf(out, "%s %s\n", NAME_LINE, reg->nomeLinha);
    else
        fprintf(out, "%s %s\n", NAME_LINE, NO_VALUE);

    if(reg->tamanhoCor != 0)
        fprintf(out, "%s %s\n", DESCRIBE_COLOR, reg->corLinha);
    else
        fprintf(out, "%s %s\n", DESCRIBE_COLOR, NO_VALUE);

    fprintf(out, "%s ", ACCEPT);
    switch(reg->aceitaCartao){
        case 'S':
            fprintf(out, "%s\n\n", YES);
            break;
        case 'N':
            fprintf(out, "%s\n\n", NO);
            break;
        case 'F':
            fprintf(out, "%s\n\n", WEEKEND);
            break;
        default:
            fprintf(out, "%s\n\n", NO_VALUE);
            break;
    }
}

static void deallocate_vehicle_strings(DBVehicleRegister v){
    free(v.categoria);
    free(v.modelo);
}

static void deallocate_bus_line_strings(DBBusLineRegister b){
    free(b.nomeLinha);
    free(b.corLinha);
}

bool read_vehicle_register(FILE *fp, DBVehicleRegister *reg) {
    ASSERT(fread(&reg->removido, 1, 1, fp));
    ASSERT(fread(&reg->tamanhoRegistro, 4, 1, fp));
    ASSERT(fread(&reg->prefixo, 5, 1, fp));
    ASSERT(fread(&reg->data, 10, 1, fp));
    ASSERT(fread(&reg->quantidadeLugares, 4, 1, fp));
    ASSERT(fread(&reg->codLinha, 4, 1, fp));
    ASSERT(fread(&reg->tamanhoModelo, 4, 1, fp));
    reg->modelo = malloc(reg->tamanhoModelo + 1);
    reg->modelo[reg->tamanhoModelo] = '\0';
    if (reg->tamanhoModelo > 0)
        ASSERT(fread(reg->modelo, reg->tamanhoModelo, 1, fp));

    ASSERT(fread(&reg->tamanhoCategoria, 4, 1, fp));
    reg->categoria = malloc(reg->tamanhoCategoria + 1);
    reg->categoria[reg->tamanhoCategoria] = '\0';
    if (reg->tamanhoCategoria > 0)
        ASSERT(fread(reg->categoria, reg->tamanhoCategoria, 1, fp));

    return true;
}

bool read_bus_line_register(FILE *fp, DBBusLineRegister *reg){
    ASSERT(fread(&reg->removido, 1, 1, fp));
    ASSERT(fread(&reg->tamanhoRegistro, 4, 1, fp));
    ASSERT(fread(&reg->codLinha, 4, 1, fp));
    ASSERT(fread(&reg->aceitaCartao, 1, 1, fp));
    ASSERT(fread(&reg->tamanhoNome, 4, 1, fp));
    reg->nomeLinha = malloc(reg->tamanhoNome + 1);
    reg->nomeLinha[reg->tamanhoNome] = '\0';
    if (reg->tamanhoNome > 0)
        ASSERT(fread(reg->nomeLinha, reg->tamanhoNome, 1, fp));

    ASSERT(fread(&reg->tamanhoCor, 4, 1, fp));
    reg->corLinha = malloc(reg->tamanhoCor + 1);
    reg->corLinha[reg->tamanhoCor] = '\0';
    if (reg->tamanhoCor > 0)
        ASSERT(fread(reg->corLinha, reg->tamanhoCor, 1, fp));

    return true;
}

bool check_vehicle_field_equals(const DBVehicleRegister *reg, const char *campo, const char *valor){
    if(strcmp(campo, "prefixo") == 0)
        return strstr(reg->prefixo, valor) != NULL;
    else if(strcmp(campo, "data") == 0)
        return strcmp(valor, reg->data) == 0;
    else if(strcmp(campo, "quantidadeLugares") == 0)
        return reg->quantidadeLugares == (int)strtol(valor, NULL, 10);
    else if(strcmp(campo, "modelo") == 0)
        return strcmp(valor, reg->modelo) == 0;
    else if(strcmp(campo, "categoria") == 0)
        return strcmp(valor, reg->categoria) == 0;

    // Nunca deveria acontecer
    fprintf(stderr, "Erro: Invalid field.");
    exit(0);
}

bool check_bus_line_field_equals(const DBBusLineRegister *reg, const char *campo, const char *valor){
    if(strcmp(campo, "codLinha") == 0)
        return reg->codLinha == (int)strtol(valor, NULL, 10);
    else if(strcmp(campo, "aceitaCartao") == 0)
        return *valor == reg->aceitaCartao;
    else if(strcmp(campo, "nomeLinha") == 0)
        return strcmp(valor, reg->nomeLinha) == 0;
    else if(strcmp(campo, "corLinha") == 0)
        return strcmp(valor, reg->corLinha) == 0;

    // Nunca deveria acontecer
    fprintf(stderr, "Erro: Invalid field.");
    exit(0);
}

bool check_file(FILE *fp){
    if(!fp){
        fprintf(stderr, ERROR_FOUND);
        fclose(fp);
        return false;
    }
    else{
        return true;
    }
}

bool SELECT_FROM_WHERE_FILE(const char *from_file, const char *where_field, const char *equals_to){
    FILE *fp = fopen(from_file, "rb");

    if (!check_file(fp)) return false;

    bool print = (where_field == NULL);
    DBVehicleHeader header;
    if(read_header_vehicle(fp, &header)){
        DBVehicleRegister reg;
        while(read_vehicle_register(fp, &reg)){
            if(where_field != NULL && equals_to != NULL)
                print = check_vehicle_field_equals(&reg, where_field, equals_to);

            if(reg.removido != '0' && print)
                print_vehicle(stdout, &reg);

            deallocate_vehicle_strings(reg);
        }
        fclose(fp);
        return true;
    }
    else{
        printf("%s", NO_REGISTER);
        fclose(fp);
        return false;
    }
}

bool SELECT_FROM_WHERE_LINE(const char *from_file, const char *where_field, const char *equals_to){
    FILE *fp = fopen(from_file, "rb");

    if(!check_file(fp)) return false;

    bool print = (where_field == NULL);
    DBBusLineHeader header;
    if(read_header_bus_line(fp, &header)){
        DBBusLineRegister reg;
        while(read_bus_line_register(fp, &reg)){
            if(where_field != NULL && equals_to != NULL)
                print = check_bus_line_field_equals(&reg, where_field, equals_to);

            if(reg.removido != '0' && print)
                print_bus_line(stdout, &reg);

            deallocate_bus_line_strings(reg);
        }
        fclose(fp);
        return true;
    }
    else{
        fprintf(stderr, "%s", ERROR_FOUND);
        fclose(fp);
        return false;
    }
}
