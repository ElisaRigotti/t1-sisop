/* Feature test macro para clock_gettime e struct timespec */
#define _POSIX_C_SOURCE 199309L

/*
 * conta-objetos-sequencial.c
 *
 * Contagem sequencial de objetos em uma matriz binária,
 * utilizando conectividade 8 e flood fill iterativo (BFS).
 *
 * Compilacao:
 *   cc -std=c89 -Wall -Wextra -pedantic conta-objetos-sequencial.c \
 *      -o conta-objetos-sequencial
 *
 * Uso:
 *   ./conta-objetos-sequencial <arquivo_matriz>
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_LINHAS  10000
#define MAX_COLUNAS 10000

static int matriz[MAX_LINHAS][MAX_COLUNAS];
static int visitado[MAX_LINHAS][MAX_COLUNAS];

/* Fila para BFS (tamanho max: todas as células) */
static int fila_r[MAX_LINHAS * MAX_COLUNAS];
static int fila_c[MAX_LINHAS * MAX_COLUNAS];

/* Oito direcoes: horizontal, vertical e diagonal */
static const int dr[] = {-1, -1, -1,  0, 0,  1, 1, 1};
static const int dc[] = {-1,  0,  1, -1, 1, -1, 0, 1};

static int linhas, colunas;

/* ------------------------------------------------------------------ */
/* Leitura da matriz a partir de arquivo                              */
/* ------------------------------------------------------------------ */
static int carregar_matriz(const char *caminho)
{
    FILE *fp;
    int i, j;

    fp = fopen(caminho, "r");
    if (!fp) {
        perror("Erro ao abrir arquivo");
        return -1;
    }

    if (fscanf(fp, "%d %d", &linhas, &colunas) != 2) {
        fprintf(stderr, "Formato inválido: esperava linhas e colunas.\n");
        fclose(fp);
        return -1;
    }

    if (linhas <= 0 || linhas > MAX_LINHAS ||
        colunas <= 0 || colunas > MAX_COLUNAS) {
        fprintf(stderr, "Dimensões fora do intervalo permitido.\n");
        fclose(fp);
        return -1;
    }

    for (i = 0; i < linhas; i++) {
        for (j = 0; j < colunas; j++) {
            if (fscanf(fp, "%d", &matriz[i][j]) != 1) {
                fprintf(stderr, "Erro ao ler célula [%d][%d].\n", i, j);
                fclose(fp);
                return -1;
            }
        }
    }

    fclose(fp);
    return 0;
}

/* ------------------------------------------------------------------ */
/* BFS iterativa a partir de (sr, sc)                                 */
/* ------------------------------------------------------------------ */
static void bfs(int sr, int sc)
{
    int inicio = 0, fim = 0;
    int r, c, k, nr, nc;

    fila_r[fim] = sr;
    fila_c[fim] = sc;
    fim++;
    visitado[sr][sc] = 1;

    while (inicio < fim) {
        r = fila_r[inicio];
        c = fila_c[inicio];
        inicio++;

        for (k = 0; k < 8; k++) {
            nr = r + dr[k];
            nc = c + dc[k];
            if (nr >= 0 && nr < linhas && nc >= 0 && nc < colunas &&
                matriz[nr][nc] == 1 && !visitado[nr][nc]) {
                visitado[nr][nc] = 1;
                fila_r[fim] = nr;
                fila_c[fim] = nc;
                fim++;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Contagem de componentes conexos                                    */
/* ------------------------------------------------------------------ */
static int contar_objetos(void)
{
    int objetos = 0;
    int i, j;

    for (i = 0; i < linhas; i++) {
        for (j = 0; j < colunas; j++) {
            visitado[i][j] = 0;
        }
    }

    for (i = 0; i < linhas; i++) {
        for (j = 0; j < colunas; j++) {
            if (matriz[i][j] == 1 && !visitado[i][j]) {
                bfs(i, j);
                objetos++;
            }
        }
    }

    return objetos;
}

/* ------------------------------------------------------------------ */
/* main                                                               */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
    int objetos;
    struct timespec t0, t1;
    double tempo_ms;

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <arquivo_matriz>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (carregar_matriz(argv[1]) != 0) {
        return EXIT_FAILURE;
    }

    clock_gettime(CLOCK_MONOTONIC, &t0);
    objetos = contar_objetos();
    clock_gettime(CLOCK_MONOTONIC, &t1);

    tempo_ms = (t1.tv_sec - t0.tv_sec) * 1000.0
             + (t1.tv_nsec - t0.tv_nsec) / 1e6;

    printf("Matriz: %d x %d\n", linhas, colunas);
    printf("Objetos encontrados: %d\n", objetos);
    printf("Tempo: %.3f ms\n", tempo_ms);

    return EXIT_SUCCESS;
}