/* Feature test macro para srand/rand */
#define _POSIX_C_SOURCE 199309L

/*
 * gerar_matriz.c
 *
 * Gerador de matrizes binarias aleatorias para testes de desempenho.
 *
 * Uso:
 *   ./gerar_matriz <linhas> <colunas> <densidade%> [semente]
 *
 * A densidade controla a proporcao aproximada de celulas com valor 1.
 * A semente (seed) e opcional e permite reproducibilidade.
 *
 * Saida: stdout no formato esperado pelo programa de contagem.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{
    int linhas, colunas, densidade;
    unsigned int semente;
    int i, j;

    if (argc < 4 || argc > 5) {
        fprintf(stderr,
                "Uso: %s <linhas> <colunas> <densidade%%> [semente]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    linhas    = atoi(argv[1]);
    colunas   = atoi(argv[2]);
    densidade = atoi(argv[3]);

    if (linhas <= 0 || colunas <= 0 ||
        densidade < 0 || densidade > 100) {
        fprintf(stderr, "Parametros invalidos.\n");
        return EXIT_FAILURE;
    }

    if (argc == 5) {
        semente = (unsigned int)atoi(argv[4]);
    } else {
        semente = (unsigned int)time(NULL);
    }
    srand(semente);

    printf("%d %d\n", linhas, colunas);
    for (i = 0; i < linhas; i++) {
        for (j = 0; j < colunas; j++) {
            if (j > 0) printf(" ");
            printf("%d", (rand() % 100) < densidade ? 1 : 0);
        }
        printf("\n");
    }

    return EXIT_SUCCESS;
}