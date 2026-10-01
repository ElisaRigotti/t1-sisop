#!/usr/bin/env python3
"""
Gera gráficos de desempenho a partir do benchmark.csv.
Saída: results/grafico_tempo.png, results/grafico_speedup.png, results/grafico_eficiencia.png
"""

import csv
import os
from collections import defaultdict

# Tentar usar matplotlib; se nao disponivel, gerar dados para gnuplot
try:
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    HAS_MPL = True
except ImportError:
    HAS_MPL = False

def ler_dados(caminho):
    """Lê o CSV e retorna medias agrupadas por (matriz, versão, threads)."""
    tempos = defaultdict(list)
    with open(caminho, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            chave = (row['matriz'], row['versao'], int(row['threads']))
            tempos[chave].append(float(row['tempo_ms']))

    medias = {}
    for chave, vals in tempos.items():
        medias[chave] = sum(vals) / len(vals)
    return medias

def calcular_metricas(medias):
    """Calcula speedup e eficiencia."""
    # Pegar matrizes unicas
    matrizes = sorted(set(k[0] for k in medias.keys()))
    threads_list = sorted(set(k[2] for k in medias.keys() if k[1] == 'paralelo'))

    resultados = {}
    for mat in matrizes:
        t_seq = medias.get((mat, 'sequencial', 1), None)
        if t_seq is None:
            continue
        resultados[mat] = {
            't_seq': t_seq,
            'threads': [],
            'tempos': [],
            'speedups': [],
            'eficiencias': []
        }
        for t in threads_list:
            t_par = medias.get((mat, 'paralelo', t), None)
            if t_par is None:
                continue
            speedup = t_seq / t_par
            eficiencia = speedup / t
            resultados[mat]['threads'].append(t)
            resultados[mat]['tempos'].append(t_par)
            resultados[mat]['speedups'].append(speedup)
            resultados[mat]['eficiencias'].append(eficiencia)

    return resultados

def gerar_graficos_mpl(resultados, out_dir):
    """Gera gráficos com matplotlib."""
    cores = ['#2196F3', '#4CAF50', '#FF9800', '#E91E63']
    marcadores = ['o', 's', '^', 'D']

    matrizes = sorted(resultados.keys())

    # Labels mais legiveis
    labels = {}
    for m in matrizes:
        dim = m.replace('matriz_', '').replace('x', ' x ')
        labels[m] = dim

    # --- Grafico 1: Tempo de execucao ---
    fig, ax = plt.subplots(figsize=(8, 5))
    for i, mat in enumerate(matrizes):
        r = resultados[mat]
        # Incluir sequencial como referencia
        all_threads = [0] + r['threads']
        all_tempos = [r['t_seq']] + r['tempos']
        ax.plot(all_threads, all_tempos,
                marker=marcadores[i % len(marcadores)],
                color=cores[i % len(cores)],
                label=labels[mat], linewidth=2, markersize=8)
    ax.set_xlabel('Número de Threads', fontsize=12)
    ax.set_ylabel('Tempo (ms)', fontsize=12)
    ax.set_title('Tempo de Execução vs Número de Threads', fontsize=14)
    ax.set_xticks([0, 1, 2, 4, 8])
    ax.set_xticklabels(['Seq', '1', '2', '4', '8'])
    ax.legend(title='Dimensão')
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(os.path.join(out_dir, 'grafico_tempo.png'), dpi=150)
    plt.close(fig)

    # --- Grafico 2: Speedup ---
    fig, ax = plt.subplots(figsize=(8, 5))
    # Linha ideal
    max_t = max(max(r['threads']) for r in resultados.values())
    ax.plot([1, max_t], [1, max_t], 'k--', alpha=0.4, label='Ideal')
    for i, mat in enumerate(matrizes):
        r = resultados[mat]
        ax.plot(r['threads'], r['speedups'],
                marker=marcadores[i % len(marcadores)],
                color=cores[i % len(cores)],
                label=labels[mat], linewidth=2, markersize=8)
    ax.set_xlabel('Número de Threads', fontsize=12)
    ax.set_ylabel('Speedup S(p) = T_seq / T_par(p)', fontsize=12)
    ax.set_title('Speedup vs Número de Threads', fontsize=14)
    ax.set_xticks([1, 2, 4, 8])
    ax.legend(title='Dimensão')
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(os.path.join(out_dir, 'grafico_speedup.png'), dpi=150)
    plt.close(fig)

    # --- Grafico 3: Eficiencia ---
    fig, ax = plt.subplots(figsize=(8, 5))
    ax.axhline(y=1.0, color='k', linestyle='--', alpha=0.4, label='Ideal')
    for i, mat in enumerate(matrizes):
        r = resultados[mat]
        ax.plot(r['threads'], r['eficiencias'],
                marker=marcadores[i % len(marcadores)],
                color=cores[i % len(cores)],
                label=labels[mat], linewidth=2, markersize=8)
    ax.set_xlabel('Número de Threads', fontsize=12)
    ax.set_ylabel('Eficiência E(p) = S(p) / p', fontsize=12)
    ax.set_title('Eficiência vs Número de Threads', fontsize=14)
    ax.set_xticks([1, 2, 4, 8])
    ax.set_ylim(0, 1.5)
    ax.legend(title='Dimensão')
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(os.path.join(out_dir, 'grafico_eficiencia.png'), dpi=150)
    plt.close(fig)

    print("Gráficos gerados em", out_dir)

def gerar_tabela_resumo(resultados, out_dir):
    """Gera tabela resumo em CSV."""
    caminho = os.path.join(out_dir, 'resumo_desempenho.csv')
    with open(caminho, 'w') as f:
        f.write('matriz,t_seq_ms,threads,t_par_ms,speedup,eficiencia\n')
        for mat in sorted(resultados.keys()):
            r = resultados[mat]
            for i, t in enumerate(r['threads']):
                f.write('{},{:.3f},{},{:.3f},{:.3f},{:.3f}\n'.format(
                    mat, r['t_seq'], t, r['tempos'][i],
                    r['speedups'][i], r['eficiencias'][i]))
    print("Resumo salvo em", caminho)

if __name__ == '__main__':
    csv_path = 'results/benchmark.csv'
    out_dir = 'results'

    medias = ler_dados(csv_path)
    resultados = calcular_metricas(medias)

    gerar_tabela_resumo(resultados, out_dir)

    if HAS_MPL:
        gerar_graficos_mpl(resultados, out_dir)
    else:
        print("matplotlib nãp disponivel. Instalando...")
        import subprocess
        subprocess.check_call(['pip3', 'install', 'matplotlib', '--break-system-packages', '-q'])
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        HAS_MPL = True
        gerar_graficos_mpl(resultados, out_dir)