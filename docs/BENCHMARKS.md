# Mesures

Tous les chiffres de cette page sortent des CSV de `results/`, produits
par `make bench` et régénérés en tableaux par `make report`. Aucun n'est
saisi à la main.

```
date        : 2026-08-04T16:05:21Z
commit      : 8306bbf-dirty
machine     : Darwin 25.5.0 arm64
compilation : cc -std=c11 -Wall -Wextra -O2
version cc  : Apple clang version 17.0.0 (clang-1700.6.3.2)
SEED=1 SCALE=1 ASTAR_BYTES=536870912 IDA_NODES=4000000000
```

> **À lire avant les tableaux.** Pour A\*, `expanded` compte des états
> distincts ; pour IDA\*, il compte des *visites*, ré-explorations
> comprises, cumulées sur toutes les itérations. Les deux colonnes n'ont
> pas le même sens selon l'algorithme.

## A\* : ce que rapporte une heuristique

Mêmes instances pour les quatre, moitié de mélange pair et moitié impair.

| heuristique | nœuds développés (médiane) | nœuds générés | temps (ms) | rapport à Manhattan |
|---|---:|---:|---:|---:|
| `zero` | 90 182 | 243 453 | 17.772 | 175.28× |
| `misplaced` | 6 148 | 16 630 | 1.095 | 11.95× |
| `manhattan` | 514 | 1 374 | 0.058 | 1.00× |
| `linear` | 250 | 670 | 0.056 | 0.49× |

```
./bin/taquin --algo astar --heuristic manhattan --runs 500 --shuffle 200 --seed 1
```

## BFS contre A\*

| algorithme | nœuds développés (médiane) | open list max | temps (ms) |
|---|---:|---:|---:|
| `bfs` | 90 512 | 23 980 | 9.018 |
| `astar` | 514 | 302 | 0.058 |

**A\* développe 176× moins de nœuds que le BFS**, à solution identique — les deux sont optimaux, c'est l'oracle qui le vérifie.

## Les structures de données : avant et après

`list` = les listes chaînées du squelette (`popBest` en O(n), `onList` en
O(n)). `fast` = tas binaire et table de hachage.

| structures | nœuds générés | temps total (s) | débit (nœuds/s) | mémoire pic (Ko) |
|---|---:|---:|---:|---:|
| `list` | 879 046 | 7.180 | 122 428 | 1 222 |
| `fast` | 442 534 | 0.022 | 20 359 496 | 800 |

**166× de débit**, à longueur de solution identique.

## Jeter un doublon sans comparer son `g`

`naive` est ce que faisait le code d'origine. `gcompare` compare les `g`
et remonte le meilleur chemin.

| politique | instances | non optimales | coups perdus au total |
|---|---:|---:|---:|
| `naive` | 1000 | 116 (11.6 %) | 232 |
| `gcompare` | 1000 | 0 (0.0 %) | 0 |

## Le 15-puzzle : A\* meurt en mémoire, IDA\* meurt en temps

Chacun est borné par la ressource qu'il épuise réellement : A\* par
`--max-bytes`, IDA\* par `--max-nodes`. Un plafond unique en nœuds
inverserait le résultat, puisque IDA\* les cumule sur toutes ses
itérations.

| mélange | algo | résolues | longueur médiane | mémoire pic médiane | temps médian (s) |
|---:|---|---:|---:|---:|---:|
| 20 | `astar` | 10/10 | 19 | 424 Ko | 0.000 |
| 20 | `ida` | 10/10 | 19 | 636 o | 0.000 |
| 40 | `astar` | 10/10 | 31 | 1 248 Ko | 0.002 |
| 40 | `ida` | 10/10 | 31 | 684 o | 0.001 |
| 60 | `astar` | 10/10 | 38 | 3 072 Ko | 0.006 |
| 60 | `ida` | 10/10 | 38 | 712 o | 0.002 |
| 80 | `astar` | 9/10 | 42 | 10 752 Ko | 0.024 |
| 80 | `ida` | 10/10 | 43 | 732 o | 0.011 |
| 100 | `astar` | 9/10 | 44 | 24 960 Ko | 0.073 |
| 100 | `ida` | 10/10 | 44 | 736 o | 0.055 |
| 140 | `astar` | 8/10 | 45 | 57 217 Ko | 0.250 |
| 140 | `ida` | 10/10 | 49 | 756 o | 0.228 |
| 200 | `astar` | 5/10 | 48 | 122 882 Ko | 0.790 |
| 200 | `ida` | 10/10 | 52 | 768 o | 1.335 |

