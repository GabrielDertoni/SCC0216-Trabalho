#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    char    prefixo[5];
    char    data[10];
    int32_t quantidadeLugares;
    int32_t codLinha;
    char    *modelo;
    char    *categoria;
} Vehicle;

typedef struct {
    int32_t codLinha;
    char    aceitaCartao;
    char    *nomeLinha;
    char    *corLinha;
} BusLine;

typedef struct {
    bool     status;
    uint64_t byteProxReg;
    uint32_t nroRegistros;
    uint32_t nroRegRemovidos;
} DBMeta;

typedef struct {
    DBMeta meta;
    char   descrevePrefixo[18];
    char   descreveData[35];
    char   descreveLugares[42];
    char   descreveLinhas[26];
    char   descreveModelo[17];
    char   descreveCategoria[20];
} DBVehicleHeader;

typedef struct {
    DBMeta meta;
    char   descreveCodigo[15];
    char   descreveCartao[13];
    char   descreveNome[13];
    char   descreveCor[24];
} DBLineHeader;

typedef struct {
    bool      removido;
    uint32_t  tamanhoRegistro;
    char      prefixo[5];
    char      data[10];
    int32_t   quantidadeLugares;
    int32_t   codLinha;
    uint32_t  tamanhoModelo;
    char      *modelo;
    uint32_t  tamanhoCategoria;
    char      *categoria;
} DBVehicleRegister;

typedef struct {
    bool      removido;
    uint32_t  tamanhoRegistro;
    uint32_t  codLinha;
    char      aceitaCartao[1];
    uint32_t  tamanhoNome;
    char      *nomeLinha;
    uint32_t  tamanhoCor;
    char      *corLinha;
} DBLineRegister;


#endif