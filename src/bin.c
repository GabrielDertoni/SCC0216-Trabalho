#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <common.h>
#include <bin.h>

// Macro que verifica se alguma expressão é igual a 1. Se ela não é, retorna
// `false` da função.
#define ASSERT(expr) if ((expr) != 1) return false

// Se assegura de que o ponteiro de arquivo está num determinado offset.
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

bool read_header_meta(DBMeta *meta, FILE *fp) {
    position(fp, 0);

    ASSERT(fread(&meta->status         , sizeof(meta->status)         , 1, fp));

    if (meta->status == '0') {
        return false;
    }

    ASSERT(fread(&meta->byteProxReg    , sizeof(meta->byteProxReg)    , 1, fp));
    ASSERT(fread(&meta->nroRegistros   , sizeof(meta->nroRegistros)   , 1, fp));
    ASSERT(fread(&meta->nroRegRemovidos, sizeof(meta->nroRegRemovidos), 1, fp));
    return true;
}
