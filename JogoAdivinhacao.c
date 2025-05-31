// JogoAdivinhacao.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

// Pipes
int pipeJogadorToGerador[2];
int pipeGeradorToJogador[2];

// Threads do Jogador
void *thread_enviar(void *arg);
void *thread_receber(void *arg);

int main() {
    // Cria os pipes
    pipe(pipeJogadorToGerador);
    pipe(pipeGeradorToJogador);

    pid_t pid = fork();

    if (pid < 0) {
        perror("Erro ao criar processo");
        exit(1);
    }

    if (pid == 0) {
        // Processo Jogador (Cliente)
        close(pipeJogadorToGerador[0]); // Fecha leitura do pipe de envio
        close(pipeGeradorToJogador[1]); // Fecha escrita do pipe de resposta

        pthread_t enviar, receber;

        pthread_create(&enviar, NULL, thread_enviar, NULL);
        pthread_create(&receber, NULL, thread_receber, NULL);

        pthread_join(enviar, NULL);
        pthread_join(receber, NULL);

        close(pipeJogadorToGerador[1]);
        close(pipeGeradorToJogador[0]);

    } else {
        // Processo Gerador (Servidor)
        close(pipeJogadorToGerador[1]); // Fecha escrita do pipe de envio
        close(pipeGeradorToJogador[0]); // Fecha leitura do pipe de resposta

        srand(time(NULL));
        int numeroSecreto = (rand() % 100) + 1;
        char buffer[128];

        while (1) {
            int palpite;
            read(pipeJogadorToGerador[0], &palpite, sizeof(int));

            if (palpite == numeroSecreto) {
                snprintf(buffer, sizeof(buffer), "✅ Acertou! O número era %d.\n", numeroSecreto);
                write(pipeGeradorToJogador[1], buffer, strlen(buffer) + 1);
                break;
            } else if (palpite < numeroSecreto) {
                strcpy(buffer, "🔼 Maior\n");
            } else {
                strcpy(buffer, "🔽 Menor\n");
            }
            write(pipeGeradorToJogador[1], buffer, strlen(buffer) + 1);
        }

        close(pipeJogadorToGerador[0]);
        close(pipeGeradorToJogador[1]);
    }

    return 0;
}

void *thread_enviar(void *arg) {
    int palpite;
    while (1) {
        printf("Digite seu palpite (1 a 100): ");
        scanf("%d", &palpite);
        write(pipeJogadorToGerador[1], &palpite, sizeof(int));
        if (palpite >= 1 && palpite <= 100) {
            break;
        }
    }
    pthread_exit(NULL);
}

void *thread_receber(void *arg) {
    char buffer[128];
    while (1) {
        read(pipeGeradorToJogador[0], buffer, sizeof(buffer));
        printf("Servidor: %s", buffer);

        if (strstr(buffer, "Acertou") != NULL) {
            break;
        }
    }
    pthread_exit(NULL);
}
