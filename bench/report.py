#!/usr/bin/env python3
"""
Lit results/*.csv et produit docs/BENCHMARKS.md plus quatre figures.

Aucun chiffre n'est saisi a la main : si un CSV change, le tableau change.
Idempotent — memes CSV en entree, memes octets en sortie.

    python3 bench/report.py        (ou : make report)
"""
import csv
import os
import statistics
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RESULTS = os.path.join(ROOT, "results")
DOCS = os.path.join(ROOT, "docs")
IMG = os.path.join(DOCS, "img")

COLORS = ["#6b7280", "#d97706", "#2563eb", "#16a34a", "#dc2626"]


def load(name):
    with open(os.path.join(RESULTS, name), newline="") as f:
        return [r for r in csv.DictReader(f)]


def ok(rows):
    return [r for r in rows if r["status"] == "ok"]


def med(rows, field):
    vals = [float(r[field]) for r in rows]
    return statistics.median(vals) if vals else 0.0


def by(rows, key):
    out = defaultdict(list)
    for r in rows:
        out[r[key]].append(r)
    return out


def curve(rows, yfield):
    """Mediane de yfield par longueur optimale. Au moins 3 points par abscisse."""
    acc = defaultdict(list)
    for r in rows:
        acc[int(r["solution_len"])].append(float(r[yfield]))
    pts = [(k, statistics.median(v)) for k, v in sorted(acc.items()) if len(v) >= 3]
    return [p[0] for p in pts], [p[1] for p in pts]


def figure(path, series, title, xlabel, ylabel, logy=True):
    fig, ax = plt.subplots(figsize=(8, 4.4), dpi=140)
    for i, (label, xs, ys) in enumerate(series):
        if not xs:
            continue
        ax.plot(xs, ys, marker="o", ms=4, lw=2, label=label,
                color=COLORS[i % len(COLORS)])
    if logy:
        ax.set_yscale("log")
    ax.set_title(title, fontsize=11)
    ax.set_xlabel(xlabel, fontsize=9)
    ax.set_ylabel(ylabel, fontsize=9)
    ax.grid(True, which="both", lw=0.4, alpha=0.4)
    ax.legend(fontsize=9, frameon=False)
    fig.tight_layout()
    os.makedirs(IMG, exist_ok=True)
    fig.savefig(path, transparent=False, facecolor="white")
    plt.close(fig)
    print("ecrit", os.path.relpath(path, ROOT))


def fmt(v, dec=0):
    return f"{v:,.{dec}f}".replace(",", " ")


def main():
    out = []
    w = out.append

    w("# Mesures\n")
    w("Tous les chiffres de cette page sortent des CSV de `results/`, produits")
    w("par `make bench` et régénérés en tableaux par `make report`. Aucun n'est")
    w("saisi à la main.\n")
    try:
        with open(os.path.join(RESULTS, "ENVIRONMENT.txt")) as f:
            w("```\n" + f.read().strip() + "\n```\n")
    except OSError:
        pass

    w("> **À lire avant les tableaux.** Pour A\\*, `expanded` compte des états")
    w("> distincts ; pour IDA\\*, il compte des *visites*, ré-explorations")
    w("> comprises, cumulées sur toutes les itérations. Les deux colonnes n'ont")
    w("> pas le même sens selon l'algorithme.\n")

    # ---- 1. heuristiques ---------------------------------------------------
    rows = ok(load("astar-heuristics.csv"))
    groups = by(rows, "heuristic")
    order = ["zero", "misplaced", "manhattan", "linear"]
    base = med(groups.get("manhattan", []), "expanded") or 1.0

    w("## A\\* : ce que rapporte une heuristique\n")
    w("Mêmes instances pour les quatre, moitié de mélange pair et moitié impair.\n")
    w("| heuristique | nœuds développés (médiane) | nœuds générés | temps (ms) | rapport à Manhattan |")
    w("|---|---:|---:|---:|---:|")
    for h in order:
        g = groups.get(h, [])
        if not g:
            continue
        e = med(g, "expanded")
        w(f"| `{h}` | {fmt(e)} | {fmt(med(g,'generated'))} | "
          f"{med(g,'seconds')*1e3:.3f} | {e/base:.2f}× |")
    w("")
    w("```")
    w("./bin/taquin --algo astar --heuristic manhattan --runs 500 --shuffle 200 --seed 1")
    w("```\n")

    figure(os.path.join(IMG, "nodes-by-heuristic.png"),
           [(h, *curve(groups.get(h, []), "expanded")) for h in order],
           "A* : nœuds développés selon l'heuristique (8-puzzle)",
           "longueur de la solution optimale", "nœuds développés (médiane, log)")

    # ---- 2. BFS vs A* ------------------------------------------------------
    rows = ok(load("bfs-vs-astar.csv"))
    g = by(rows, "algo")
    w("## BFS contre A\\*\n")
    w("| algorithme | nœuds développés (médiane) | open list max | temps (ms) |")
    w("|---|---:|---:|---:|")
    for a in ["bfs", "astar"]:
        if a in g:
            w(f"| `{a}` | {fmt(med(g[a],'expanded'))} | {fmt(med(g[a],'open_max'))} | "
              f"{med(g[a],'seconds')*1e3:.3f} |")
    if "bfs" in g and "astar" in g:
        r = med(g["bfs"], "expanded") / max(med(g["astar"], "expanded"), 1.0)
        w("")
        w(f"**A\\* développe {r:.0f}× moins de nœuds que le BFS**, à solution "
          "identique — les deux sont optimaux, c'est l'oracle qui le vérifie.\n")

    figure(os.path.join(IMG, "bfs-vs-astar.png"),
           [(a, *curve(g.get(a, []), "expanded")) for a in ["bfs", "astar"]],
           "BFS contre A*/Manhattan (8-puzzle)",
           "longueur de la solution optimale", "nœuds développés (médiane, log)")

    # ---- 3. structures de donnees -----------------------------------------
    rows = ok(load("impl-list-vs-fast.csv"))
    g = by(rows, "impl")
    w("## Les structures de données : avant et après\n")
    w("`list` = les listes chaînées du squelette (`popBest` en O(n), `onList` en")
    w("O(n)). `fast` = tas binaire et table de hachage.\n")
    w("| structures | nœuds générés | temps total (s) | débit (nœuds/s) | mémoire pic (Ko) |")
    w("|---|---:|---:|---:|---:|")
    for impl in ["list", "fast"]:
        if impl not in g:
            continue
        gen = sum(float(r["generated"]) for r in g[impl])
        sec = sum(float(r["seconds"]) for r in g[impl])
        w(f"| `{impl}` | {fmt(gen)} | {sec:.3f} | {fmt(gen/sec if sec else 0)} | "
          f"{fmt(max(float(r['bytes_peak']) for r in g[impl])/1024)} |")
    if "list" in g and "fast" in g:
        def thr(k):
            gen = sum(float(r["generated"]) for r in g[k])
            sec = sum(float(r["seconds"]) for r in g[k])
            return gen / sec if sec else 0.0
        w("")
        w(f"**{thr('fast')/max(thr('list'),1):.0f}× de débit**, à longueur de "
          "solution identique.\n")

    def throughput_curve(rows_):
        acc = defaultdict(list)
        for r in rows_:
            s = float(r["seconds"])
            if s > 0:
                acc[int(r["solution_len"])].append(float(r["generated"]) / s)
        pts = [(k, statistics.median(v)) for k, v in sorted(acc.items()) if len(v) >= 3]
        return [p[0] for p in pts], [p[1] for p in pts]

    figure(os.path.join(IMG, "impl-throughput.png"),
           [(i, *throughput_curve(g.get(i, []))) for i in ["list", "fast"]],
           "Débit selon les structures de données : le O(n²) rendu visible",
           "longueur de la solution optimale", "nœuds générés par seconde (log)")

    # ---- 4. politique de doublons -----------------------------------------
    rows = load("dup-policy.csv")
    g = by(rows, "dup")
    w("## Jeter un doublon sans comparer son `g`\n")
    w("`naive` est ce que faisait le code d'origine. `gcompare` compare les `g`")
    w("et remonte le meilleur chemin.\n")
    w("| politique | instances | non optimales | coups perdus au total |")
    w("|---|---:|---:|---:|")
    for d in ["naive", "gcompare"]:
        if d not in g:
            continue
        rr = [r for r in ok(g[d]) if int(r["optimal"]) >= 0]
        bad = [r for r in rr if int(r["solution_len"]) != int(r["optimal"])]
        lost = sum(int(r["solution_len"]) - int(r["optimal"]) for r in bad)
        pct = 100.0 * len(bad) / len(rr) if rr else 0.0
        w(f"| `{d}` | {len(rr)} | {len(bad)} ({pct:.1f} %) | {lost} |")
    w("")

    # ---- 5. le 15-puzzle ---------------------------------------------------
    rows = load("puzzle15.csv")
    w("## Le 15-puzzle : A\\* meurt en mémoire, IDA\\* meurt en temps\n")
    w("Chacun est borné par la ressource qu'il épuise réellement : A\\* par")
    w("`--max-bytes`, IDA\\* par `--max-nodes`. Un plafond unique en nœuds")
    w("inverserait le résultat, puisque IDA\\* les cumule sur toutes ses")
    w("itérations.\n")
    w("| mélange | algo | résolues | longueur médiane | mémoire pic médiane | temps médian (s) |")
    w("|---:|---|---:|---:|---:|---:|")
    for sh in sorted({int(r["shuffle"]) for r in rows}):
        for a in ["astar", "ida"]:
            sel = [r for r in rows if int(r["shuffle"]) == sh and r["algo"] == a]
            if not sel:
                continue
            good = ok(sel)
            mem = med(good, "bytes_peak") / 1024 if good else 0
            unit = f"{mem:,.0f} Ko".replace(",", " ") if mem >= 1 else f"{mem*1024:.0f} o"
            w(f"| {sh} | `{a}` | {len(good)}/{len(sel)} | "
              f"{med(good,'solution_len'):.0f} | {unit} | {med(good,'seconds'):.3f} |")
    w("")

    figure(os.path.join(IMG, "astar-vs-ida-memory.png"),
           [(a, *curve(ok([r for r in rows if r["algo"] == a]), "bytes_peak"))
            for a in ["astar", "ida"]],
           "15-puzzle : mémoire pic, A* contre IDA*",
           "longueur de la solution optimale", "octets alloués (médiane, log)")

    path = os.path.join(DOCS, "BENCHMARKS.md")
    os.makedirs(DOCS, exist_ok=True)
    with open(path, "w") as f:
        f.write("\n".join(out) + "\n")
    print("ecrit", os.path.relpath(path, ROOT))


if __name__ == "__main__":
    main()
