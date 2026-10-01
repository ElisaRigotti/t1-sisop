#!/bin/sh
# benchmark.sh — Roda benchmarks sequencial e paralelo
# Uso: ./tools/benchmark.sh
# Resultados salvos em results/benchmark.csv

set -e

MATRIZES="tests/adicionais/matriz_100x100.txt \
tests/adicionais/matriz_500x500.txt \
tests/adicionais/matriz_1000x1000.txt \
tests/adicionais/matriz_2000x2000.txt"

THREADS="1 2 4 8"
REPETICOES=5

mkdir -p results

SAIDA="results/benchmark.csv"
echo "matriz,dimensao,versao,threads,objetos,tempo_ms,execucao" > "$SAIDA"

for MAT in $MATRIZES; do
    DIM=$(head -1 "$MAT" | tr -s ' ')
    NOME=$(basename "$MAT" .txt)
    echo "=== $NOME ($DIM) ==="

    # Sequencial
    for r in $(seq 1 $REPETICOES); do
        RESULT=$(./conta-objetos-sequencial "$MAT")
        OBJ=$(echo "$RESULT" | grep "Objetos" | awk '{print $NF}')
        TEMPO=$(echo "$RESULT" | grep "Tempo" | awk '{print $2}')
        echo "$NOME,$DIM,sequencial,1,$OBJ,$TEMPO,$r" >> "$SAIDA"
        printf "  seq  run%d: %s obj, %s ms\n" "$r" "$OBJ" "$TEMPO"
    done

    # Paralelo
    for T in $THREADS; do
        for r in $(seq 1 $REPETICOES); do
            RESULT=$(./conta-objetos-paralelo "$MAT" "$T")
            OBJ=$(echo "$RESULT" | grep "Objetos" | awk '{print $NF}')
            TEMPO=$(echo "$RESULT" | grep "Tempo" | awk '{print $2}')
            echo "$NOME,$DIM,paralelo,$T,$OBJ,$TEMPO,$r" >> "$SAIDA"
            printf "  par  t=%d run%d: %s obj, %s ms\n" "$T" "$r" "$OBJ" "$TEMPO"
        done
    done

    echo ""
done

echo "Benchmark concluido. Resultados em $SAIDA"