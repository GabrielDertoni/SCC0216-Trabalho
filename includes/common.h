/**
 * Header file que define alguns tipos e macros utilizados globalmente no
 * projeto. Esse arquivo não é genérico e define exatamente os structs de duas
 * formas:
 *      - Representação lida do CSV: Vehicle e BusLine
 *      - Representação lida do binário: DBMeta, DBVehicleHeader,
 *                                       DBBusLineHeader, DBVehicleRegister,
 *                                       DBBusLineRegister
 */


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
#define REGISTER_NOT_FOUND   "Registro inexistente.\n"

// Representação de um veículo que será lida do CSV.
typedef struct {
    char    prefixo[5];
    char    data[10];
    int32_t quantidadeLugares;
    int32_t codLinha;
    char    *modelo;
    char    *categoria;
} Vehicle;

// Representação de uma linha de ônibus que será lida do CSV.
typedef struct {
    // É uma string pois pode ser um número ou possuir um '*' no começo
    // sinalizando que o registro está removido. Por conta da forma como csv.h
    // funciona, não podemos fácilmente separar isso em dois campos.
    char codLinha[32];
    char aceitaCartao[1];
    char *nomeLinha;
    char *corLinha;
} BusLine;

// Header que é comum entre os arquivos binários e possui metadados sobre ele.
typedef struct {
    char     status;
    uint64_t byteProxReg;
    uint32_t nroRegistros;
    uint32_t nroRegRemovidos;
} DBMeta;

// Header de um arquivo binário de veículo.
typedef struct {
    DBMeta meta;
    char   descrevePrefixo[18];
    char   descreveData[35];
    char   descreveLugares[42];
    char   descreveLinhas[26];
    char   descreveModelo[17];
    char   descreveCategoria[20];
} DBVehicleHeader;

// Header de um arquivo binário de linha de ônibus.
typedef struct {
    DBMeta meta;
    char   descreveCodigo[15];
    char   descreveCartao[13];
    char   descreveNome[13];
    char   descreveCor[24];
} DBBusLineHeader;

// Registro de veículo do binário de veículo.
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

// Registro de linha de ônibus do binário de linha.
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
