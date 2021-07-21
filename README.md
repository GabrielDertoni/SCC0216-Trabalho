# CSS0215 - Trabalho

## Trabalho 1

No trabalho 1 precisamos implementar um sistema que converte os registros de um
arquivo .csv para um arquivo binário. Para trabalhar com csv de maneira
genérica, o módulo `csv` é utilizado. Ele separa os campos e constrói uma
representação na memória que espelha algum struct. Para isso é necessário
fornecer funções de _parsing_ que sejam capazes de pegar o conteúdo de um único
campo e interpretar esse campo como algum tipo, seja `int` ou `char *` mesmo.
Essas funções de parsing se encontram no `parsing`.

O módulo `parsing` fornece a configuração de dois CSVs. Com essas
configurações é possível ler corretamente os campos do csv configurado com
tratamentos de erro automático e execução eficiente. Com isso, o módulo
`csv_to_bin` é capaz de interpretar o CSV configurado e rodar uma função
específica para cada registro do CSV. Isso é feito pela função
`csv_iterate_rows` que é uma das funções de leitura do módulo `csv` e permite
que uma função seja executada para cada registro. Nesse caso a função passada
varia, podendo ser uma que interpreta veículos ou uma que interpreta linhas de
ônibus. O módulo `csv_to_bin` então é justamente a interface entre a parte do
csv que é configurado pelo `parsing` e do módulo que mexe com binário (`bin`).

Além disso, as operações todas que mexem com o binário diretamente estão no
módulo `bin`. Inclusive as de imprimir todos os registros ou imprimir alguns
registros que correspondem a critérios.

Por fim, o `main.c` chama essas funções do módulo `bin` e do módulo
`csv_to_bin`.

## Trabalho 2

No trabalho 2, o módulo `btree` implementa a estrutura de dados BTree. As
funcionalidades da especificação que são chamadas pelo `main.c` e que trabalham
com o índice de BTree estão definidos no módulo `index`. Isso inclui as
funcionalidades 13 e 14 que funcionam de maneira similar às funcionalidades 7 e 8.
Mesmo que a 7 e 8 estejam presentes em `csv_to_bin`.

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
- encontrar as funções print_vehicle() e print_bus_line() e acrescentar fprintf(stdout, "\n") depois da chamada delas. Isso porque eu alterei essas funções para se encaixarem nas definições do trabalho 3 e para que os dois outros trabalhos ainda funcionem é preciso fazer essa leve alteração.
