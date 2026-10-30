# Jogo de Adivinhação

Jogo de adivinhação de número (1 a 100) em C, desenvolvido para a disciplina de Sistemas Operacionais.

## Arquitetura

| Componente | Papel | Mecanismo |
|---|---|---|
| Processo gerador | Sorteia o número, avalia palpites e envia dicas | `fork` |
| Processo jogador | Lê palpites e exibe respostas | `pthread` (envio e recepção) |
| Comunicação | Palpites e respostas entre os processos | `pipe` (um por sentido) |

## Requisitos

- GCC
- Linux ou macOS (POSIX)

## Compilação e execução

```bash
gcc JogoAdivinhacao.c -o JogoAdivinhacao -pthread
./JogoAdivinhacao
```

## Como jogar

1. Informe um palpite entre 1 e 100.
2. Receba a dica: o número é maior ou menor.
3. Ao acertar, o jogo informa o total de tentativas e pergunta se deseja jogar novamente (`S`/`N`).

## Conceitos aplicados

Criação de processos, comunicação entre processos (IPC) com pipes e multithreading.
