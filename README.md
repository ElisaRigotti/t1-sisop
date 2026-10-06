# Contagem Paralela de Objetos em Matriz Binária

Trabalho prático da disciplina de Sistemas Operacionais (2026/2) — PUCRS, Escola Politécnica.

- LINK YOUTUBE: https://youtu.be/a6qPTX1Awys?si=ojgYFEYw6Uctn8TT

## Descrição

Implementação sequencial e paralela da contagem de componentes conexos (objetos) em uma matriz binária, utilizando **conectividade 8** (horizontal, vertical e diagonal).

- **Versão sequencial:** BFS iterativa percorrendo toda a matriz em um único fluxo de controle.
- **Versão paralela:** Pthreads com decomposição por faixas de linhas e Union-Find para consolidação de componentes que atravessam fronteiras entre regiões.

## Autoria

| Campo | Informação |
|---|---|
| Integrante 1 | Elisa Ely de Oliveira Rigotti |
| Matrícula 1 | 24106421 |
| Integrante 2 | Maria Júlia Escobar Correia de Melo |
| Matrícula 2 | 23180253 |
| Turma | 330 |

## Compilação

```bash
make          # compila ambas as versões
make sequencial   # compila apenas a versão sequencial
make paralelo     # compila apenas a versão paralela
make clean        # remove os binários
```

Requisitos: compilador C compatível com C89/C90, suporte a Pthreads, Linux ou macOS.

## Execucao

```bash
# Versão sequencial
./conta-objetos-sequencial tests/obrigatorios/exemplo1_5x5.txt

# Versão paralela (com 4 threads)
./conta-objetos-paralelo tests/obrigatorios/exemplo1_5x5.txt 4
```

## Formato da entrada

Arquivo texto com a primeira linha contendo `linhas colunas`, seguido pelos valores da matriz (0 ou 1) separados por espaço:

```
5 5
1 1 0 0 0
1 1 0 0 0
0 0 0 1 0
0 0 0 1 0
1 0 0 0 0
```

## Arquitetura da solução paralela

1. **Leitura:** thread principal lê a matriz do arquivo.
2. **Decomposição:** a matriz é dividida em faixas de linhas (row-stripe), uma por thread.
3. **Identificação local:** cada thread executa BFS iterativa na sua faixa, atribuindo rótulos únicos por região.
4. **Consolidação:** após todas as threads terminarem, a thread principal percorre as fronteiras entre faixas adjacentes e unifica rótulos com Union-Find (disjoint set union).
5. **Contagem:** conta-se o número de raízes distintas no Union-Find.

## Estrutura do repositorio

```
README.md
RELATORIO_TECNICO.md
Makefile
src/
    conta-objetos-sequencial.c
    conta-objetos-paralelo.c
tests/
    obrigatorios/       (5 matrizes do enunciado)
    adicionais/         (matrizes extras para desempenho)
tools/
    gerar_matriz.c
    benchmark.sh
    gerar_graficos.py
results/                (medições e gráficos)
slides/
    apresentacao.pdf
```

## Matrizes de teste obrigatórias

| Exemplo | Dimensoes | Objetos esperados |
|---:|---:|---:|
| 1 | 5 x 5 | 3 |
| 2 | 6 x 8 | 4 |
| 3 | 8 x 8 | 5 |
| 4 | 9 x 12 | 6 |
| 5 | 12 x 12 | 7 |

