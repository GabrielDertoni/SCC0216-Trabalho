#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <common.h>

#define WASSERT(expr)     if ((expr) != 1) return false
#define POSITION(fp, off) if (ftell(fp) != off) fseek(fp, off, SEEK_SET)

static inline DBMeta header_meta_default(uint32_t nroRegistros) {
    return (DBMeta) {
        .status = false,
        .byteProxReg = 0,
        .nroRegistros = nroRegistros,
        .nroRegRemovidos = 0,
    };
}

bool update_header_status(char new_val, FILE *fp) {
    POSITION(fp, 0);
    WASSERT(fwrite(&new_val, sizeof(new_val), 1, fp));
    return true;
}

bool update_header_byte_next_reg(uint64_t byte_next_reg, FILE *fp) {
    POSITION(fp, 1);
    WASSERT(fwrite(&byte_next_reg, sizeof(byte_next_reg), 1, fp));
    return true;
}

bool update_header_n_reg(uint32_t n_reg, FILE *fp) {
    POSITION(fp, 9);
    WASSERT(fwrite(&n_reg, sizeof(n_reg), 1, fp));
    return true;
}

bool update_header_n_reg_removed(uint32_t n_reg_removed, FILE *fp) {
    POSITION(fp, 13);
    WASSERT(fwrite(&n_reg_removed, sizeof(n_reg_removed), 1, fp));
    return true;
}

bool write_header_meta(DBMeta meta, FILE *fp) {
    WASSERT(fwrite(&meta.status           , sizeof(meta.status)         , 1, fp));
    WASSERT(fwrite(&meta.byteProxReg      , sizeof(meta.byteProxReg)    , 1, fp));
    WASSERT(fwrite(&meta.nroRegistros     , sizeof(meta.nroRegistros)   , 1, fp));
    WASSERT(fwrite(&meta.nroRegRemovidos  , sizeof(meta.nroRegRemovidos), 1, fp));
    return true;
}

bool write_vehicles_header(DBVehicleHeader header, FILE *fp) {
    WASSERT(write_header_meta(header.meta, fp));
    WASSERT(fwrite(&header.descrevePrefixo  , sizeof(header.descrevePrefixo)  , 1, fp));
    WASSERT(fwrite(&header.descreveData     , sizeof(header.descreveData)     , 1, fp));
    WASSERT(fwrite(&header.descreveLugares  , sizeof(header.descreveLugares)  , 1, fp));
    WASSERT(fwrite(&header.descreveLinhas   , sizeof(header.descreveLinhas)   , 1, fp));
    WASSERT(fwrite(&header.descreveModelo   , sizeof(header.descreveModelo)   , 1, fp));
    WASSERT(fwrite(&header.descreveCategoria, sizeof(header.descreveCategoria), 1, fp));
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

    uint32_t tamanhoModelo = vehicle->modelo ? strlen(vehicle->modelo) : 0;
    uint32_t tamanhoCategoria = vehicle->categoria ? strlen(vehicle->categoria) : 0;
    uint32_t tamanhoRegistro = 0;
    tamanhoRegistro += sizeof(vehicle->prefixo);
    tamanhoRegistro += sizeof(vehicle->data);
    tamanhoRegistro += sizeof(vehicle->quantidadeLugares);
    tamanhoRegistro += sizeof(vehicle->codLinha);
    tamanhoRegistro += sizeof(tamanhoModelo);
    tamanhoRegistro += tamanhoModelo;
    tamanhoRegistro += sizeof(tamanhoCategoria);
    tamanhoRegistro += tamanhoCategoria;

    WASSERT(fwrite(&removido                  , sizeof(removido)                  , 1, fp));
    WASSERT(fwrite(&tamanhoRegistro           , sizeof(tamanhoRegistro)           , 1, fp));
    WASSERT(fwrite(prefixo                    , sizeof(vehicle->prefixo)          , 1, fp));
    WASSERT(fwrite(vehicle->data              , sizeof(vehicle->data)             , 1, fp));
    WASSERT(fwrite(&vehicle->quantidadeLugares, sizeof(vehicle->quantidadeLugares), 1, fp));
    WASSERT(fwrite(&vehicle->codLinha         , sizeof(vehicle->codLinha)         , 1, fp));
    WASSERT(fwrite(&tamanhoModelo             , sizeof(tamanhoModelo)             , 1, fp));

    if (tamanhoModelo > 0) {
        WASSERT(fwrite(vehicle->modelo        , tamanhoModelo * sizeof(char)      , 1, fp));
    }

    WASSERT(fwrite(&tamanhoCategoria          , sizeof(tamanhoCategoria)          , 1, fp));

    if (tamanhoCategoria > 0) {
        WASSERT(fwrite(vehicle->categoria     , tamanhoCategoria * sizeof(char)   , 1, fp));
    }

    return true;
}

bool write_bus_lines_header(DBBusLineHeader header, FILE *fp) {
    WASSERT(write_header_meta(header.meta, fp));
    WASSERT(fwrite(&header.descreveCodigo, sizeof(header.descreveCodigo), 1, fp));
    WASSERT(fwrite(&header.descreveCartao, sizeof(header.descreveCartao), 1, fp));
    WASSERT(fwrite(&header.descreveNome  , sizeof(header.descreveNome  ), 1, fp));
    WASSERT(fwrite(&header.descreveCor   , sizeof(header.descreveCor   ), 1, fp));
    return true;
}

bool write_bus_line(BusLine line, FILE *fp) {
    int32_t codLinha;

    char removido;
    if (line.codLinha[0] == REMOVED_MARKER) {
        removido = '0';
        codLinha = (int)strtol(&line.codLinha[1], NULL, 10);
    } else {
        removido = '1';
        codLinha = (int)strtol(line.codLinha, NULL, 10);
    }

    uint32_t tamanhoNome = line.nomeLinha ? strlen(line.nomeLinha) : 0;
    uint32_t tamanhoCor  = line.corLinha ? strlen(line.corLinha) : 0;
    uint32_t tamanhoRegistro = 0;
    tamanhoRegistro += sizeof(codLinha);
    tamanhoRegistro += sizeof(line.aceitaCartao);
    tamanhoRegistro += sizeof(tamanhoNome);
    tamanhoRegistro += tamanhoNome;
    tamanhoRegistro += sizeof(tamanhoCor);
    tamanhoRegistro += tamanhoCor;

    WASSERT(fwrite(&removido         , sizeof(removido)          , 1, fp));
    WASSERT(fwrite(&tamanhoRegistro  , sizeof(tamanhoRegistro)   , 1, fp));
    WASSERT(fwrite(&codLinha         , sizeof(codLinha)          , 1, fp));
    WASSERT(fwrite(&line.aceitaCartao, sizeof(line.aceitaCartao) , 1, fp));
    WASSERT(fwrite(&tamanhoNome      , sizeof(tamanhoNome)       , 1, fp));

    if (tamanhoNome > 0) {
        WASSERT(fwrite(line.nomeLinha, tamanhoNome * sizeof(char), 1, fp));
    }

    WASSERT(fwrite(&tamanhoCor       , sizeof(tamanhoCor)        , 1, fp));

    if (tamanhoCor > 0) {
        WASSERT(fwrite(line.corLinha , tamanhoCor * sizeof(char) , 1, fp));
    }
    return true;
}
