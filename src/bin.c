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

bool update_header_n_reg_removed(uint32_t n_reg_removed, FILE *fp) {
    position(fp, 13);
    ASSERT(fwrite(&n_reg_removed, sizeof(n_reg_removed), 1, fp));
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

// bool CREATE_TABLE_FILE(char *file_name_in, char *file_name_out){
//     return vehicle_csv_to_bin(file_name_in);
// }

// bool CREATE_TABLE_LINE(char *file_name_in, char *file_name_out){

// }

bool read_meta(FILE *fp, DBMeta *meta){
    ASSERT(fread(&meta->status, 1, 1, fp));
    if(meta->status == '0')
        return false;
    ASSERT(fread(&meta->byteProxReg, sizeof(long), 1, fp));
    ASSERT(fread(&meta->nroRegistros, sizeof(int), 1, fp));
    ASSERT(fread(&meta->nroRegRemovidos, sizeof(int), 1, fp));
    return true;
}

bool read_header_file(FILE *fp, DBVehicleHeader *header){
    if(!read_meta(fp, &header->meta))
        return false;
    ASSERT(fread(&header->descrevePrefixo, 18, 1, fp));
    ASSERT(fread(&header->descreveData, 35, 1, fp));
    ASSERT(fread(&header->descreveLugares, 42, 1, fp));
    ASSERT(fread(&header->descreveLinhas, 26, 1, fp));
    ASSERT(fread(&header->descreveModelo, 17, 1, fp));
    ASSERT(fread(&header->descreveCategoria, 20, 1, fp));
    return true;
}

bool read_header_line(FILE *fp, DBBusLineHeader *header){
    if(!read_meta(fp, &header->meta));
    ASSERT(fread(&header->descreveCodigo, 15, 1, fp));
    ASSERT(fread(&header->descreveCartao, 13, 1, fp));
    ASSERT(fread(&header->descreveNome, 13, 1, fp));
    ASSERT(fread(&header->descreveCor, 24, 1, fp));
    return true;
}

void print_result_file(FILE *out, DBVehicleRegister v){
    fprintf(out, "%s %.5s\n", PREFIX, v.prefixo);
    if(v.tamanhoModelo != 0)
        fprintf(out, "%s %s\n", MODEL, v.modelo);
    else
        fprintf(out, "%s %s\n", MODEL, NO_VALUE);
    if(v.tamanhoCategoria != 0)
        fprintf(out, "%s %s\n", CATEGORY, v.categoria);
    else
        fprintf(out, "%s %s\n", CATEGORY, NO_VALUE);
    if(strlen(v.data) != 0){
        char *months[12] = {"janeiro", "fevereiro", "março", "abril", "maio", "junho", "julho", "agosto", "setembro", "outubro", "novembro", "dezembro"};
        char *year = strtok(v.data, "-"), *month = strtok(NULL, "-"), *day = strtok(NULL, "-");
        fprintf(out, "%s %.2s de %s de %s\n", DATE, day, months[atoi(month)-1], year);
    }
    else
        fprintf(out, "%s %s\n", DATE, NO_VALUE);
    if(v.quantidadeLugares != -1)
        fprintf(out, "%s %d\n\n", PLACES, v.quantidadeLugares);
    else
        fprintf(out, "%s %s\n\n", PLACES, NO_VALUE);
}

void print_result_line(FILE *out, DBBusLineRegister b){
    fprintf(out, "%s %d\n", COD_LINHA, b.codLinha);
    if(b.tamanhoNome != 0)
        fprintf(out, "%s %s\n", NAME_LINE, b.nomeLinha);
    else
        fprintf(out, "%s %s\n", NAME_LINE, NO_VALUE);
    // fprintf(out, "tamanho: %d\n", b.tamanhoCor);
    if(b.tamanhoCor != 0)
        fprintf(out, "%s %s\n", DESCRIBE_COLOR, b.corLinha);
    else
        fprintf(out, "%s %s\n", DESCRIBE_COLOR, NO_VALUE);
    fprintf(out, "%s ", ACCEPT);
    switch(b.aceitaCartao){
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

void deallocate_file_strings(DBVehicleRegister v){
    free(v.categoria);
    free(v.modelo);
}

void deallocate_line_strings(DBBusLineRegister b){
    free(b.nomeLinha);
    free(b.corLinha);
}

DBVehicleRegister read_file_registers(FILE *fp, DBVehicleRegister v){
    fread(&v.tamanhoRegistro, 4, 1, fp);
    fread(&v.prefixo, 5, 1, fp);
    fread(&v.data, 10, 1, fp);
    fread(&v.quantidadeLugares, 4, 1, fp);
    fread(&v.codLinha, 4, 1, fp);
    fread(&v.tamanhoModelo, 4, 1, fp);
    v.modelo = malloc(v.tamanhoModelo+1);
    fread(v.modelo, v.tamanhoModelo, 1, fp);
    v.modelo[v.tamanhoModelo] = 0;
    fread(&v.tamanhoCategoria, 4, 1, fp);
    v.categoria = malloc(v.tamanhoCategoria+1);
    fread(v.categoria, v.tamanhoCategoria, 1, fp);
    return v;
}

DBBusLineRegister read_line_register(FILE *fp, DBBusLineRegister b){
    fread(&b.tamanhoRegistro, 4, 1, fp);
    fread(&b.codLinha, 4, 1, fp);
    fread(&b.aceitaCartao, 1, 1, fp);
    fread(&b.tamanhoNome, 4, 1, fp);
    b.nomeLinha = malloc(b.tamanhoNome+1);
    fread(b.nomeLinha, b.tamanhoNome, 1, fp);
    b.nomeLinha[b.tamanhoNome] = 0;
    // printf("nome: %s\n", b.nomeLinha);
    fread(&b.tamanhoCor, 4, 1, fp);
    b.corLinha = malloc(b.tamanhoCor+1);
    fread(b.corLinha, b.tamanhoCor, 1, fp);
    b.corLinha[b.tamanhoCor] = 0;
    return b;
}

bool check_validate_file(DBVehicleRegister v, char *campo, char *valor){
    // scan_quote_string(valor);
    // printf("comp: %d\n", strcmp(campo, "prefixo"));
    if(strcmp(campo, "prefixo") == 0)
        return strstr(v.prefixo, valor) != NULL;
    else if(strcmp(campo, "data") == 0)
        return strcmp(valor, v.data) == 0;
    else if(strcmp(campo, "quantidadeLugares") == 0)
        return v.quantidadeLugares == atoi(valor);
    else if(strcmp(campo, "modelo") == 0)
        return strcmp(valor, v.modelo) == 0;
    else if(strcmp(campo, "categoria") == 0)
        return strcmp(valor, v.categoria) == 0;
    else
        return false; // Sei lá (???)
}

bool check_validate_line(DBBusLineRegister b, char *campo, char *valor){
    if(strcmp(campo, "codLinha") == 0)
        return b.codLinha == atoi(valor);
    else if(strcmp(campo, "aceitaCartao") == 0)
        return *valor == b.aceitaCartao;
    else if(strcmp(campo, "nomeLinha") == 0)
        return strcmp(valor, b.nomeLinha) == 0;
    else if(strcmp(campo, "corLinha") == 0)
        return strcmp(valor, b.corLinha) == 0;
    else
        return false; // Sei lá (???)
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

bool SELECT_FROM_WHERE_FILE(char *from_file, char *where_campo, char *where_valor){
    FILE *fp = fopen(from_file, "rb");
    if(check_file(fp)){
        bool print = (where_campo == NULL);
        DBVehicleHeader header;
        if(read_header_file(fp, &header)){
            DBVehicleRegister v;
            while(fread(&v.removido, 1, 1, fp) == 1){
                v = read_file_registers(fp, v);
                if(where_campo != NULL && where_valor != NULL)
                    print = check_validate_file(v, where_campo, where_valor);
                if(v.removido != '0' && print)
                    print_result_file(stdout, v);
                deallocate_file_strings(v);
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
    return false;
}

bool SELECT_FROM_WHERE_LINE(char *from_file, char *where_campo, char *where_valor){
    FILE *fp = fopen(from_file, "rb");
    if(check_file(fp)){
        bool print = (where_campo == NULL);
        DBBusLineHeader header;
        if(read_header_line(fp, &header)){
            DBBusLineRegister b;
            while(fread(&b.removido, 1, 1, fp) == 1){
                b = read_line_register(fp, b);
                if(where_campo != NULL && where_valor != NULL)
                    print = check_validate_line(b, where_campo, where_valor);
                // printf("removido: %c print: %d\n", b.removido, print);
                if(b.removido != '0' && print){
                    print_result_line(stdout, b);
                }
                deallocate_line_strings(b);
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
    return false;
}
