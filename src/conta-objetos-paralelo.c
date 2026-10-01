/* Feature test macros para clock_gettime e Pthreads */
#define _POSIX_C_SOURCE 199309L

/*
 * conta-objetos-paralelo.c
 *
 * Contagem paralela de objetos (componentes conexos) em uma matriz
 * binária, utilizando conectividade 8, Pthreads e Union-Find para
 * consolidação de componentes que atravessam fronteiras entre regiões.
 *
 * Estratégia: decomposição por faixas de linhas (row-stripe).
 * Cada thread recebe uma faixa contígua de linhas, identifica
 * componentes locais via BFS iterativa com rótulos únicos, e após
 * todas as threads terminarem, a thread principal percorre as
 * fronteiras e unifica componentes com Union-Find.
 *
 * Compilacao:
 *   cc -std=c89 -Wall -Wextra -pedantic -pthread \
 *      conta-objetos-paralelo.c -o conta-objetos-paralelo
 *
 * Uso:
 *   ./conta-objetos-paralelo <arquivo_matriz> <num_threads>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

/* Limites */
#define MAX_LINHAS          10000
#define MAX_COLUNAS         10000
#define MAX_THREADS         64
#define MAX_ROTULOS_POR_REG 500000
#define MAX_ROTULOS         (MAX_THREADS * MAX_ROTULOS_POR_REG)

/* Matriz de entrada e matriz de rótulos */
static int matriz[MAX_LINHAS][MAX_COLUNAS];
static int rotulo[MAX_LINHAS][MAX_COLUNAS];

static int linhas, colunas;
static int num_threads;

/* Oito direções */
static const int dr[] = {-1, -1, -1,  0, 0,  1, 1, 1};
static const int dc[] = {-1,  0,  1, -1, 1, -1, 0, 1};

/* ------------------------------------------------------------------ */
/* Union-Find                                                         */
/* ------------------------------------------------------------------ */
static int parent[MAX_ROTULOS];
static int rank_uf[MAX_ROTULOS];
static int total_rotulos;

static void uf_init(int n)
{
    int i;
    for (i = 0; i < n; i++) {
        parent[i] = i;
        rank_uf[i] = 0;
    }
}

static int uf_find(int x)
{
    while (parent[x] != x) {
        parent[x] = parent[parent[x]];  /* compressão de caminho */
        x = parent[x];
    }
    return x;
}

static void uf_union(int a, int b)
{
    a = uf_find(a);
    b = uf_find(b);
    if (a == b) return;
    if (rank_uf[a] < rank_uf[b]) {
        int t = a; a = b; b = t;
    }
    parent[b] = a;
    if (rank_uf[a] == rank_uf[b]) rank_uf[a]++;
}

/* ------------------------------------------------------------------ */
/* Estrutura de dados para cada thread                                */
/* ------------------------------------------------------------------ */
typedef struct {
    int id;
    int linha_inicio;
    int linha_fim;      /* exclusivo */
    int base_rotulo;    /* primeiro rótulo reservado para esta região */
    int prox_rotulo;    /* proximo rótulo disponível */
} thread_data_t;

static thread_data_t tdata[MAX_THREADS];

/* Fila BFS local por thread (alocada dinamicamente) */
typedef struct {
    int *fila_r;
    int *fila_c;
    int capacidade;
} fila_bfs_t;

/* ------------------------------------------------------------------ */
/* Leitura da matriz                                                  */
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
        fprintf(stderr, "Formato invalido.\n");
        fclose(fp);
        return -1;
    }

    if (linhas <= 0 || linhas > MAX_LINHAS ||
        colunas <= 0 || colunas > MAX_COLUNAS) {
        fprintf(stderr, "Dimensoes fora do intervalo.\n");
        fclose(fp);
        return -1;
    }

    for (i = 0; i < linhas; i++) {
        for (j = 0; j < colunas; j++) {
            if (fscanf(fp, "%d", &matriz[i][j]) != 1) {
                fprintf(stderr, "Erro ao ler celula [%d][%d].\n", i, j);
                fclose(fp);
                return -1;
            }
        }
    }

    fclose(fp);
    return 0;
}

/* ------------------------------------------------------------------ */
/* BFS local dentro da faixa de linhas da thread                      */
/* ------------------------------------------------------------------ */
static void bfs_local(int sr, int sc, int lab,
                       int li, int lf,
                       fila_bfs_t *f)
{
    int inicio = 0, fim = 0;
    int r, c, k, nr, nc;

    f->fila_r[fim] = sr;
    f->fila_c[fim] = sc;
    fim++;
    rotulo[sr][sc] = lab;

    while (inicio < fim) {
        r = f->fila_r[inicio];
        c = f->fila_c[inicio];
        inicio++;

        for (k = 0; k < 8; k++) {
            nr = r + dr[k];
            nc = c + dc[k];
            /* Limita a BFS a faixa da thread */
            if (nr >= li && nr < lf &&
                nc >= 0 && nc < colunas &&
                matriz[nr][nc] == 1 && rotulo[nr][nc] == 0) {
                rotulo[nr][nc] = lab;
                f->fila_r[fim] = nr;
                f->fila_c[fim] = nc;
                fim++;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Função executada por cada thread                                   */
/* ------------------------------------------------------------------ */
static void *trabalho_thread(void *arg)
{
    thread_data_t *td = (thread_data_t *)arg;
    fila_bfs_t f;
    int i, j;
    int cap;

    cap = (td->linha_fim - td->linha_inicio) * colunas;
    f.fila_r = (int *)malloc(cap * sizeof(int));
    f.fila_c = (int *)malloc(cap * sizeof(int));
    f.capacidade = cap;

    if (!f.fila_r || !f.fila_c) {
        fprintf(stderr, "Thread %d: falha na alocacao.\n", td->id);
        free(f.fila_r);
        free(f.fila_c);
        return NULL;
    }

    td->prox_rotulo = td->base_rotulo;

    for (i = td->linha_inicio; i < td->linha_fim; i++) {
        for (j = 0; j < colunas; j++) {
            if (matriz[i][j] == 1 && rotulo[i][j] == 0) {
                td->prox_rotulo++;
                bfs_local(i, j, td->prox_rotulo,
                          td->linha_inicio, td->linha_fim, &f);
            }
        }
    }

    free(f.fila_r);
    free(f.fila_c);
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Consolidação: percorre fronteiras entre faixas adjacentes           */
/* ------------------------------------------------------------------ */
static void consolidar_fronteiras(void)
{
    int t, j, d;
    int linha_borda;

    for (t = 0; t < num_threads - 1; t++) {
        linha_borda = tdata[t].linha_fim - 1; /* ultima linha da faixa t */
        /* Para cada célula na linha de borda e na primeira linha da
           faixa seguinte, verificar vizinhos nas 8 direções que
           cruzam a fronteira */
        for (j = 0; j < colunas; j++) {
            if (rotulo[linha_borda][j] == 0) continue;
            /* Verificar vizinhos na linha seguinte (3 direções) */
            for (d = 5; d < 8; d++) {
                int nr = linha_borda + dr[d];
                int nc = j + dc[d];
                if (nr >= 0 && nr < linhas &&
                    nc >= 0 && nc < colunas &&
                    rotulo[nr][nc] != 0) {
                    uf_union(rotulo[linha_borda][j], rotulo[nr][nc]);
                }
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Contagem global de componentes distintos                           */
/* ------------------------------------------------------------------ */
static int contar_componentes_globais(void)
{
    int *eh_raiz;
    int count = 0;
    int i, j, r;

    eh_raiz = (int *)calloc(total_rotulos + 1, sizeof(int));
    if (!eh_raiz) {
        fprintf(stderr, "Falha na alocacao para contagem.\n");
        return -1;
    }

    for (i = 0; i < linhas; i++) {
        for (j = 0; j < colunas; j++) {
            if (rotulo[i][j] != 0) {
                r = uf_find(rotulo[i][j]);
                if (!eh_raiz[r]) {
                    eh_raiz[r] = 1;
                    count++;
                }
            }
        }
    }

    free(eh_raiz);
    return count;
}

/* ------------------------------------------------------------------ */
/* main                                                               */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
    pthread_t threads[MAX_THREADS];
    int t, rc;
    int linhas_por_thread, sobra, acumulado;
    int objetos;
    struct timespec t0, t1;
    double tempo_ms;

    if (argc != 3) {
        fprintf(stderr, "Uso: %s <arquivo_matriz> <num_threads>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    num_threads = atoi(argv[2]);
    if (num_threads < 1 || num_threads > MAX_THREADS) {
        fprintf(stderr, "Numero de threads deve estar entre 1 e %d.\n",
                MAX_THREADS);
        return EXIT_FAILURE;
    }

    if (carregar_matriz(argv[1]) != 0) {
        return EXIT_FAILURE;
    }

    /* Ajustar num_threads se houver mais threads do que linhas */
    if (num_threads > linhas) {
        num_threads = linhas;
    }

    /* Inicializar rótulos como 0 */
    memset(rotulo, 0, sizeof(rotulo));

    /* Distribuir faixas de linhas entre as threads */
    linhas_por_thread = linhas / num_threads;
    sobra = linhas % num_threads;
    acumulado = 0;

    for (t = 0; t < num_threads; t++) {
        tdata[t].id = t;
        tdata[t].linha_inicio = acumulado;
        tdata[t].linha_fim = acumulado + linhas_por_thread
                           + (t < sobra ? 1 : 0);
        tdata[t].base_rotulo = t * MAX_ROTULOS_POR_REG;
        acumulado = tdata[t].linha_fim;
    }

    total_rotulos = num_threads * MAX_ROTULOS_POR_REG;
    uf_init(total_rotulos + 1);

    /* Medição de tempo */
    clock_gettime(CLOCK_MONOTONIC, &t0);

    /* Criar threads para identificação local */
    for (t = 0; t < num_threads; t++) {
        rc = pthread_create(&threads[t], NULL, trabalho_thread,
                            &tdata[t]);
        if (rc != 0) {
            fprintf(stderr, "Erro ao criar thread %d (rc=%d).\n", t, rc);
            return EXIT_FAILURE;
        }
    }

    /* Aguardar todas as threads */
    for (t = 0; t < num_threads; t++) {
        rc = pthread_join(threads[t], NULL);
        if (rc != 0) {
            fprintf(stderr, "Erro ao juntar thread %d (rc=%d).\n", t, rc);
            return EXIT_FAILURE;
        }
    }

    /* Consolidação sequencial das fronteiras */
    consolidar_fronteiras();

    /* Contagem global */
    objetos = contar_componentes_globais();

    clock_gettime(CLOCK_MONOTONIC, &t1);
    /* Fim da medição */

    tempo_ms = (t1.tv_sec - t0.tv_sec) * 1000.0
             + (t1.tv_nsec - t0.tv_nsec) / 1e6;

    printf("Matriz: %d x %d\n", linhas, colunas);
    printf("Threads: %d\n", num_threads);
    printf("Objetos encontrados: %d\n", objetos);
    printf("Tempo: %.3f ms\n", tempo_ms);

    return EXIT_SUCCESS;
}
