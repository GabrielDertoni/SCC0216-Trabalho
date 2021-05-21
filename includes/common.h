#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>

#define META_SIZE            17
#define VEHICLE_HEADER_SIZE  (158 + META_SIZE)
#define BUS_LINE_HEADER_SIZE ( 65 + META_SIZE)

#define NULL_VAL             "NULO"
#define REMOVED_MARKER       '*'
#define ERROR_FOUND          "Falha no processamento do arquivo.\n"

typedef struct {
    char    prefixo[5];
    char    data[10];
    int32_t quantidadeLugares;
    int32_t codLinha;
    char    *modelo;
    char    *categoria;
} Vehicle;

typedef struct {
    char codLinha[32];
    char aceitaCartao[1];
    char *nomeLinha;
    char *corLinha;
} BusLine;

typedef struct {
    char     status;
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
} DBBusLineHeader;


typedef struct {
    char      removido;
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
    char      removido;
    uint32_t  tamanhoRegistro;
    uint32_t  codLinha;
    char      aceitaCartao;
    uint32_t  tamanhoNome;
    char      *nomeLinha;
    uint32_t  tamanhoCor;
    char      *corLinha;
} DBBusLineRegister;



#endif
