#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <common.h>

#define WASSERT(expr)           if ((expr) != 1) return false
#define WASSERT_OR(expr, expr2) ({ if ((expr) != 1) { expr2; return false; } })
#define POSITION(fp, off)       if (ftell(fp) != off) fseek(fp, off, SEEK_SET)

static inline DBMeta header_meta_default(uint32_t nroRegistros) {
    return (DBMeta) {
        .status = false,
        .byteProxReg = 0,
        .nroRegistros = nroRegistros,
        .nroRegRemovidos = 0,
    };
}

bool update_header_status(bool new_val, FILE *fp) {
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

bool write_vehicles_header(uint32_t nroRegistros, FILE *fp) {
    DBVehicleHeader header = {
        .meta              = header_meta_default(nroRegistros),
        .descrevePrefixo   = "@@@",
        .descreveData      = "@@@",
        .descreveLugares   = "@@@",
        .descreveLinhas    = "@@@",
        .descreveModelo    = "@@@",
        .descreveCategoria = "@@@",
    };

    WASSERT(write_header_meta(header.meta, fp));
    WASSERT(fwrite(&header.descrevePrefixo  , sizeof(header.descrevePrefixo)  , 1, fp));
    WASSERT(fwrite(&header.descreveData     , sizeof(header.descreveData)     , 1, fp));
    WASSERT(fwrite(&header.descreveLugares  , sizeof(header.descreveLugares)  , 1, fp));
    WASSERT(fwrite(&header.descreveLinhas   , sizeof(header.descreveLinhas)   , 1, fp));
    WASSERT(fwrite(&header.descreveModelo   , sizeof(header.descreveModelo)   , 1, fp));
    WASSERT(fwrite(&header.descreveCategoria, sizeof(header.descreveCategoria), 1, fp));
    return true;
}

bool write_vehicle(Vehicle vehicle, FILE *fp) {
    bool removido = false;
    uint32_t tamanhoModelo = strlen(vehicle.modelo);
    uint32_t tamanhoCategoria = strlen(vehicle.categoria);
    uint32_t tamanhoRegistro = 0;
    tamanhoRegistro += sizeof(vehicle.prefixo);
    tamanhoRegistro += sizeof(vehicle.data);
    tamanhoRegistro += sizeof(vehicle.quantidadeLugares);
    tamanhoRegistro += sizeof(vehicle.codLinha);
    tamanhoRegistro += tamanhoModelo;
    tamanhoRegistro += tamanhoCategoria;

    WASSERT(fwrite(&removido                 , sizeof(removido)                 , 1, fp));
    WASSERT(fwrite(&tamanhoRegistro          , sizeof(tamanhoRegistro)          , 1, fp));
    WASSERT(fwrite(vehicle.prefixo           , sizeof(vehicle.prefixo)          , 1, fp));
    WASSERT(fwrite(vehicle.data              , sizeof(vehicle.data)             , 1, fp));
    WASSERT(fwrite(&vehicle.quantidadeLugares, sizeof(vehicle.quantidadeLugares), 1, fp));
    WASSERT(fwrite(&vehicle.codLinha         , sizeof(vehicle.codLinha)         , 1, fp));
    WASSERT(fwrite(&tamanhoModelo            , sizeof(tamanhoModelo)            , 1, fp));
    WASSERT(fwrite(vehicle.modelo            , tamanhoModelo * sizeof(char)     , 1, fp));
    WASSERT(fwrite(&tamanhoCategoria         , sizeof(tamanhoCategoria)         , 1, fp));
    WASSERT(fwrite(vehicle.categoria         , tamanhoCategoria * sizeof(char)  , 1, fp));
    return true;
}

bool write_vehicles(Vehicle *vehicles, uint32_t n_vehicles, const char *fname) {
    FILE *fp = fopen(fname, "w");
    if (!fp) return false;

    WASSERT_OR(write_vehicles_header(n_vehicles, fp), fclose(fp));

    for (int i = 0; i < n_vehicles; i++) {
        WASSERT_OR(write_vehicle(vehicles[i], fp), fclose(fp));
    }
    uint64_t curr_pos = ftell(fp);
    WASSERT_OR(update_header_byte_next_reg(curr_pos, fp), fclose(fp));

    update_header_status(true, fp);
    fclose(fp);
    return true;
}

bool write_bus_lines_header(uint32_t nroRegistros, FILE *fp) {
    DBLineHeader header = {
        .meta           = header_meta_default(nroRegistros),
        .descreveCodigo = "@@@",
        .descreveCartao = "@@@",
        .descreveNome   = "@@@",
        .descreveCor    = "@@@",
    };

    WASSERT(write_header_meta(header.meta, fp));
    WASSERT(fwrite(&header.descreveCodigo, sizeof(header.descreveCodigo), 1, fp));
    WASSERT(fwrite(&header.descreveCartao, sizeof(header.descreveCartao), 1, fp));
    WASSERT(fwrite(&header.descreveNome  , sizeof(header.descreveNome  ), 1, fp));
    WASSERT(fwrite(&header.descreveCor   , sizeof(header.descreveCor   ), 1, fp));
    return true;
}

bool write_bus_line(BusLine line, FILE *fp) {
    bool removido = false;
    uint32_t tamanhoNome = strlen(line.nomeLinha);
    uint32_t tamanhoCor  = strlen(line.corLinha);
    uint32_t tamanhoRegistro = 0;
    tamanhoRegistro += sizeof(line.codLinha);
    tamanhoRegistro += sizeof(line.aceitaCartao);
    tamanhoRegistro += tamanhoNome;
    tamanhoRegistro += tamanhoCor;

    WASSERT(fwrite(&removido         , sizeof(removido)          , 1, fp));
    WASSERT(fwrite(&tamanhoRegistro  , sizeof(tamanhoRegistro)   , 1, fp));
    WASSERT(fwrite(&line.codLinha    , sizeof(line.codLinha)     , 1, fp));
    WASSERT(fwrite(&line.aceitaCartao, sizeof(line.aceitaCartao) , 1, fp));
    WASSERT(fwrite(&tamanhoNome      , sizeof(tamanhoNome)       , 1, fp));
    WASSERT(fwrite(&line.nomeLinha   , tamanhoNome * sizeof(char), 1, fp));
    WASSERT(fwrite(&tamanhoCor       , sizeof(tamanhoCor)        , 1, fp));
    WASSERT(fwrite(&line.corLinha    , tamanhoCor * sizeof(char) , 1, fp));
    return true;
}

bool write_bus_lines(BusLine *lines, uint32_t n_lines, const char *fname) {
    FILE *fp = fopen(fname, "w");
    if (!fp) return false;

    if (!write_bus_lines_header(n_lines, fp)) {
        fclose(fp);
        return false;
    }

    for (int i = 0; i < n_lines; i++) {
        if(!write_bus_line(lines[i], fp)) {
            fclose(fp);
            return false;
        }
    }
    uint64_t curr_pos = ftell(fp);
    if(!update_header_byte_next_reg(curr_pos, fp)) {
        fclose(fp);
        return false;
    }

    update_header_status(true, fp);
    fclose(fp);
    return true;
}
