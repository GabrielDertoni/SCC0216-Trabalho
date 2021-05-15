
## Uso do Makefile

### Compilando e executando o binário

Gerar o binário é relativamente simples, apenas use `make` ou `make all`. Para
executar use `make run`. Esse modo de compilação não possui flags de debug. Para
isso compile com `make debug`.

O binário gerado a partir de uma compilação normal fica armazenado em
`BUILD_DIR` definido no Makefile. Por padrão `target/build`. Já o binário com
flag de debug fica em `DEBUG_DIR` que possui como padrão `target/debug`.

### Testes em módulos

Para rodar todos os testes e obter uma visão geral deles, use `make test`. Esse
comando omite a saída padrão e de erros dos testes e apenas fornece estatísticas
de quantos testes passaram e quantos falharam.

Para ver o resultado de um teste específico use `make <test name>`, onde `<test
name>` é o nome do teste a ser executado. Esse nome deve ser igual ao nome do
arquivo na pasta `tests`.

Além disso os testes compilados ficarão armazenados em `TEST_DIR` definido no
Makefile. Por padrão essa localização é `target/test`.

Para somente compilar os testes mas não rodar nenhum, use `make build_tests`.

## Questões abertas
- É melhor calcular qual será o `byteProxReg` antes mesmo de escrever no
    arquivo? Ou vale mais a pena escrever primeiro e ver onde o ponteiro
    termina?

## TODO
- Renomear `Column` para `CSVColumn`
