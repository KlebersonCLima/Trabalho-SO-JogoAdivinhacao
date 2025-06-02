#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

// Pipes criados
int pipeJogadorToGerador[2];
int pipeGeradorToJogador[2];

// Threads para enviar o palpite e receber resposta.
void *ThreadEnviarPalpite(void *arg);
void *ThreadReceberResposta(void *arg);

int main()
{
    pipe(pipeJogadorToGerador);
    pipe(pipeGeradorToJogador);

    pid_t ProcessoFilho = fork();

    if (ProcessoFilho < 0)
    {
        perror("Processo filho não foi criado");
        exit(1);
    }

    if (ProcessoFilho == 0)
    {
        // Processo Jogador - Cliente
        close(pipeJogadorToGerador[0]); // Fecha leitura do pipe de envio
        close(pipeGeradorToJogador[1]); // Fecha escrita do pipe de resposta

        pthread_t enviar, receber;

        pthread_create(&enviar, NULL, ThreadEnviarPalpite, NULL);
        pthread_create(&receber, NULL, ThreadReceberResposta, NULL);

        pthread_join(enviar, NULL);
        pthread_join(receber, NULL);

        close(pipeJogadorToGerador[1]);
        close(pipeGeradorToJogador[0]);
    }
    else
    {
        // Processo Gerador - Servidor
        close(pipeJogadorToGerador[1]); // Fecha escrita do pipe de envio
        close(pipeGeradorToJogador[0]); // Fecha leitura do pipe de resposta

        srand(time(NULL));
        int numeroSecreto = (rand() % 100) + 1;
        char buffer[128];

        int palpite;
        while (1) // fica lendo o palpite do jogador.
        {
            char Continuar[10];
            read(pipeJogadorToGerador[0], &palpite, sizeof(int));

            if (palpite == numeroSecreto)
            {
                snprintf(buffer, sizeof(buffer), "✅ Acertou! O número era %d.\n", numeroSecreto);
                write(pipeGeradorToJogador[1], buffer, strlen(buffer) + 1);

                printf("Deseja continuar o jogo?");
                scanf("%s", Continuar);
                if (strcmp(Continuar, "S") == 0 || strcmp(Continuar, "s") == 0 || strcmp(Continuar, "Sim") == 0 || strcmp(Continuar, "sim") == 0)
                {
                }
                else
                {
                    break;
                }
            }
            else if (palpite < numeroSecreto)
            {
                strcpy(buffer, "🔼 Maior\n");
            }
            else
            {
                strcpy(buffer, "🔽 Menor\n");
            }
            write(pipeGeradorToJogador[1], buffer, strlen(buffer) + 1);
        }

        close(pipeJogadorToGerador[0]);
        close(pipeGeradorToJogador[1]);
    }

    return 0;
}

void *ThreadEnviarPalpite(void *arg)
{
    int palpite;
    while (1)
    {
        printf("Digite seu palpite (1 a 100): ");
        scanf("%d", &palpite);
        write(pipeJogadorToGerador[1], &palpite, sizeof(int));
        if (palpite >= 1 && palpite <= 100) // Se for entre ele deve continuar para que o usuario consiga dar mais palpites.
        {
            continue;
        }
    }
    pthread_exit(NULL);
}

void *ThreadReceberResposta(void *arg)
{
    char buffer[128];
    while (1)
    {
        read(pipeGeradorToJogador[0], buffer, sizeof(buffer));
        printf("\n\nServidor: %s", buffer);

        if (strstr(buffer, "Acertou") != NULL)
        {
            break;
        }
    }
    pthread_exit(NULL);
}
