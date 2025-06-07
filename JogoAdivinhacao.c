#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

// Pipes para comunicação entre processos
int pipeJogadorToGerador[2];
int pipeGeradorToJogador[2];
int pipeControlFlow[2]; // Pipe adicional para controle de fluxo

// Variáveis globais para controle
int jogoAtivo = 1;
int acertou = 0;
int aguardandoPalpite = 0;
int aguardandoContinuar = 0;

// Threads
void *ThreadEnviarPalpite(void *arg);
void *ThreadReceberResposta(void *arg);

void limparTela()
{
    system("clear || cls");
}

// Função personalizada para exibição do cabeçalho
void exibirCabecalho()
{
    printf("╔═══════════════════════════════════════╗\n");
    printf("║         🎯 JOGO DE ADIVINHAÇÃO        ║\n");
    printf("║         Adivinhe o número de 1-100    ║\n");
    printf("╚═══════════════════════════════════════╝\n\n");
}

int main()
{

    if (pipe(pipeJogadorToGerador) == -1 ||
        pipe(pipeGeradorToJogador) == -1 ||
        pipe(pipeControlFlow) == -1)
    {
        perror("Erro ao criar pipes");
        exit(1);
    }

    pid_t ProcessoFilho = fork();

    if (ProcessoFilho < 0)
    {
        perror("Erro ao criar processo filho");
        exit(1);
    }

    if (ProcessoFilho == 0)
    {
        // ===== PROCESSO JOGADOR (CLIENTE) =====
        close(pipeJogadorToGerador[0]); // Fecha leitura do pipe de envio
        close(pipeGeradorToJogador[1]); // Fecha escrita do pipe de resposta
        close(pipeControlFlow[1]);      // Fecha escrita do pipe de controle

        limparTela();
        exibirCabecalho();

        pthread_t threadEnviar, threadReceber;

        pthread_create(&threadEnviar, NULL, ThreadEnviarPalpite, NULL);
        pthread_create(&threadReceber, NULL, ThreadReceberResposta, NULL);

        pthread_join(threadEnviar, NULL);
        pthread_join(threadReceber, NULL);

        close(pipeJogadorToGerador[1]);
        close(pipeGeradorToJogador[0]);
        close(pipeControlFlow[0]);
    }
    else
    {
        // ===== PROCESSO GERADOR (SERVIDOR) =====
        close(pipeJogadorToGerador[1]); // Fecha escrita do pipe de envio
        close(pipeGeradorToJogador[0]); // Fecha leitura do pipe de resposta
        close(pipeControlFlow[0]);      // Fecha leitura do pipe de controle

        srand(time(NULL));

        while (jogoAtivo)
        {
            int numeroSecreto = (rand() % 100) + 1;
            printf("\n🎲 Novo número gerado! (Entre 1 e 100)\n");
            printf("═════════════════════════════════════════\n");

            char buffer[256];
            int palpite;
            int tentativas = 0;

            // Enviar sinal para iniciar novo jogo
            strcpy(buffer, "NOVO_JOGO");
            write(pipeGeradorToJogador[1], buffer, strlen(buffer) + 1);

            while (1)
            {
                read(pipeJogadorToGerador[0], &palpite, sizeof(int));
                tentativas++;

                if (palpite == -1)
                {
                    jogoAtivo = 0;
                    strcpy(buffer, "SAIR");
                    write(pipeGeradorToJogador[1], buffer, strlen(buffer) + 1);
                    break;
                }

                if (palpite == numeroSecreto)
                {
                    snprintf(buffer, sizeof(buffer),
                             "ACERTOU|✅ Parabéns! Você acertou o número %d em %d tentativa(s)!",
                             numeroSecreto, tentativas);
                    write(pipeGeradorToJogador[1], buffer, strlen(buffer) + 1);

                    // Aguardar resposta se quer continuar
                    char continuar;
                    read(pipeJogadorToGerador[0], &continuar, sizeof(char));

                    if (continuar == 'N' || continuar == 'n')
                    {
                        jogoAtivo = 0;
                        strcpy(buffer, "SAIR");
                        write(pipeGeradorToJogador[1], buffer, strlen(buffer) + 1);
                    }
                    break;
                }
                else if (palpite < numeroSecreto)
                {
                    snprintf(buffer, sizeof(buffer), "DICA|🔼 O número é MAIOR que %d", palpite);
                }
                else
                {
                    snprintf(buffer, sizeof(buffer), "DICA|🔽 O número é MENOR que %d", palpite);
                }

                write(pipeGeradorToJogador[1], buffer, strlen(buffer) + 1);
            }
        }

        printf("\nServidor encerrando... Obrigado por jogar!\n");

        close(pipeJogadorToGerador[0]);
        close(pipeGeradorToJogador[1]);
        close(pipeControlFlow[1]);
    }

    return 0;
}

void *ThreadEnviarPalpite(void *arg)
{
    int palpite;
    char resposta;

    while (jogoAtivo)
    {
        if (aguardandoContinuar)
        {
            printf("🤔 Deseja jogar novamente? (S/N): ");
            fflush(stdout);
            scanf(" %c", &resposta);

            write(pipeJogadorToGerador[1], &resposta, sizeof(char));
            aguardandoContinuar = 0;

            if (resposta == 'N' || resposta == 'n')
            {
                jogoAtivo = 0;
                break;
            }

            // Aguardar início do novo jogo
            while (!aguardandoPalpite && jogoAtivo)
            {
                usleep(50000);
            }
            continue;
        }

        if (aguardandoPalpite)
        {
            printf("💭 Digite seu palpite (1 a 100): ");
            fflush(stdout);

            if (scanf("%d", &palpite) != 1)
            {
                // Limpar buffer em caso de entrada inválida
                while (getchar() != '\n')
                    ;
                printf("❌ Por favor, digite apenas números!\n\n");
                continue;
            }

            if (palpite < 1 || palpite > 100)
            {
                printf("❌ Por favor, digite um número entre 1 e 100!\n\n");
                continue;
            }

            write(pipeJogadorToGerador[1], &palpite, sizeof(int));
            aguardandoPalpite = 0;
        }
    }

    pthread_exit(NULL);
}

void *ThreadReceberResposta(void *arg)
{
    char buffer[256];

    while (jogoAtivo)
    {
        int bytesLidos = read(pipeGeradorToJogador[0], buffer, sizeof(buffer) - 1);
        if (bytesLidos <= 0)
            continue;

        buffer[bytesLidos] = '\0';

        if (strstr(buffer, "NOVO_JOGO") != NULL)
        {
            printf("Novo jogo iniciado! Tente adivinhar o número!\n");
            printf("─────────────────────────────────────────────────\n");
            aguardandoPalpite = 1; // Libera para pedir palpite
            continue;
        }

        if (strstr(buffer, "SAIR") != NULL)
        {
            jogoAtivo = 0;
            printf("\n🎯 Fim de jogo!\n");
            printf("═══════════════════════════════════════\n");
            break;
        }

        // Processar mensagens com separador |
        char *tipo = strtok(buffer, "|");
        char *mensagem = strtok(NULL, "|");

        if (tipo && mensagem)
        {
            if (strcmp(tipo, "ACERTOU") == 0)
            {
                printf("\n%s\n", mensagem);
                printf("─────────────────────────────────────────────────\n");
                aguardandoContinuar = 1; // Libera para perguntar se quer continuar
            }
            else if (strcmp(tipo, "DICA") == 0)
            {
                printf("\n%s\n", mensagem);
                printf("─────────────────────────────────────────────────\n");
                aguardandoPalpite = 1; // Libera para próximo palpite
            }
        }
    }

    pthread_exit(NULL);
}