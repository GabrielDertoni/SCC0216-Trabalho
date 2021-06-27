
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

## TODO
- Renomear `Column` para `CSVColumn`.
- Mudar de `parsing.h` para `csv_to_bin.h`?
- Testar mudanças no código do trabalho 1 contra os casos de trabalho 1 para
    verificar se tudo continua funcionando corretamente.
- `veiculo1.bin` parece ter o número errado de registros em seu cabeçalho,
    verificar.
- Alocação de memória desnecessária quando lendo uma string de tamanho 0 no
    binário de veículo ou de linha de ônibus.
- Adicionar nomes e números usp no cabeçalho da `main.c`
- Adicionar `const` a ponteiros que nunca precisam ser modificados no `bin.c`.
