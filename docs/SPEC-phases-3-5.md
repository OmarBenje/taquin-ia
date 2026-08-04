# Spec — Phases 3 à 5 : heuristiques comparées, IDA\*, 15-puzzle, écrit

État : à implémenter
Branche : `phases-0-5`
Rédigée le : 2026-08-04

---

## Contexte

Les phases 0 à 2 sont livrées (7 commits, `make test` vert). Le dépôt sait
aujourd'hui générer une instance reproductible (`--seed`), la résoudre en BFS ou
en A\*, compter ce qu'il fait, et écrire une ligne de CSV par instance. Les
structures de données ont été refaites : tas binaire à la place de `popBest`,
table de hachage à la place de `onList`.

Ce qui manque est ce qui transforme le dépôt en démonstration :

1. **Personne ne voit les chiffres.** Ils existent dans des CSV que personne ne
   lance. Il n'y a ni tableau ni graphe dans le dépôt.
2. **A\* meurt en mémoire sur le 15-puzzle et rien ne le montre.** C'est le seul
   endroit où le projet cesse d'être un TP.
3. **Le README décrit encore le projet d'avant.** Il ne dit pas ce qui vient de
   l'école et ce qui vient de l'auteur, et ne répond à aucune des six questions
   qu'un jury posera.

**Qui est concerné :** l'auteur, en entretien. Le lecteur cible est un
ingénieur qui ouvre le dépôt pendant cinq minutes et veut savoir si les
affirmations sont mesurées ou récitées.

**Pourquoi maintenant :** sans les phases 3 à 5, le travail des phases 1 et 2
n'est visible que par quelqu'un qui compile et lance le banc lui-même.

---

## État actuel (vérifié le 2026-08-04)

### Ce qui existe

| Fichier | Rôle | Vérifié |
|---|---|---|
| `src/search.h:9` | `algo_id` = `{ALGO_BFS, ALGO_ASTAR}` | pas d'IDA\* |
| `src/search.h:19` | `impl_id` = `{IMPL_LIST, IMPL_FAST}` | les deux marchent |
| `src/heuristic.h:24` | `heuristic_id` = `{H_ZERO, H_MISPLACED, H_MANHATTAN, H_LINEAR}` | les quatre marchent |
| `src/puzzle.h:15` | `WH_BOARD` surchargeable à la compilation | **le 4x4 compile déjà** |
| `src/taquin.c` | banc CLI, écrit le CSV | opérationnel |
| `Makefile:15` | cibles `taquin`, `taquin_vs`, `taquin_baseline` | pas de cible 4x4 |
| `results/` | vide | aucun CSV commité |
| `docs/` | ne contient que cette spec | aucun tableau, aucun graphe |
| `README.md` | 104 lignes, décrit le mode Joueur vs IA | ne mentionne ni banc, ni mesure, ni provenance |

### Mesures déjà obtenues (à publier, pas à refaire)

A\*/Manhattan, graines 1000-1029, 3x3 :

| | `--impl list` | `--impl fast` |
|---|---|---|
| longueur moyenne | 22,13 | 22,13 |
| nœuds générés | 122 291 | 66 057 |
| temps total | 0,821 s | 0,004 s |
| débit | 148 970 nœuds/s | 18 792 888 nœuds/s |

Nœuds développés par heuristique, cumul sur l'échantillon de `test_search` :
`zero` 1 221 040, `misplaced` 56 314, `manhattan` 8 720, `linear` 5 328.

`--dup naive` rend une solution non optimale sur **2 instances sur 120**.

### Le mur mémoire du 15-puzzle, mesuré

Compilation ad hoc en `-DWH_BOARD=4`, A\*/Manhattan, 3 instances par ligne,
plafond 20 M nœuds :

| mélange | longueur optimale moyenne | mémoire pic (recherche) | temps moyen | issue |
|---|---|---|---|---|
| 60 | 37,3 | 11,9 Mo | 15 ms | résolu |
| 80 | 42,0 | 71,2 Mo | 140 ms | résolu |
| 100 | 42,0 | 28,8 Mo | 40 ms | résolu |
| 200 | 50,0 | **764 Mo** | 4,1 s | **1 abandon sur 3** |

C'est le graphe de la phase 4 : une courbe qui s'arrête net.

### Environnement (contraintes réelles)

- `matplotlib` **n'est pas installé** (`python3 -c "import matplotlib"` échoue).
  Python 3.10.7 est disponible. Les graphes doivent donc être produits en SVG
  écrit à la main, bibliothèque standard seulement.
- `codex` n'est pas installé : le gate qualité de `/spec` a été sauté.

---

## Changement proposé

### P3 — Artefacts de comparaison des heuristiques

**`bench/run_bench.sh`** — un script, aucune option, entièrement déterministe.
Il produit dans `results/` :

| Fichier | Commande | Rôle |
|---|---|---|
| `astar-heuristics.csv` | `--algo astar --impl fast --heuristic {zero,misplaced,manhattan,linear} --runs 1000 --seed 1` | comparaison des heuristiques |
| `bfs-vs-astar.csv` | `--algo bfs` et `--algo astar` `--impl fast --runs 1000 --seed 1` | le facteur BFS/A\* |
| `impl-list-vs-fast.csv` | `--impl {list,fast} --runs 200 --seed 1` | avant/après structures |
| `dup-policy.csv` | `--dup {naive,gcompare} --impl {list,fast} --runs 2000 --seed 1 --verify` | taux de non-optimalité de la politique naïve |
| `puzzle15.csv` | `taquin15 --algo {astar,ida} --shuffle {20,40,...,200} --runs 20 --max-nodes 20000000` | le mur mémoire |

Le script écrit aussi `results/ENVIRONMENT.txt` (uname, version du compilateur,
date, SHA du commit) pour que les chiffres soient rattachables à une machine.

**`bench/report.py`** — bibliothèque standard uniquement. Lit `results/*.csv` et
écrit :

- `docs/BENCHMARKS.md` : tous les tableaux, en markdown, avec médiane et moyenne.
- `docs/img/nodes-by-heuristic.svg` : nœuds développés (log) par longueur optimale, une courbe par heuristique.
- `docs/img/bfs-vs-astar.svg` : nœuds développés (log) par longueur optimale, BFS contre A\*.
- `docs/img/impl-throughput.svg` : débit (nœuds/s) en fonction du nombre de nœuds générés, `list` contre `fast` — c'est ce graphe qui montre l'effondrement en O(n²).
- `docs/img/astar-vs-ida-memory.svg` : mémoire pic (log) par longueur optimale sur le 15-puzzle, A\* contre IDA\* — la courbe d'A\* s'arrête, celle d'IDA\* continue.

Les SVG sont générés par une fonction unique paramétrée (axes, échelle log,
légende, grille) ; pas de dépendance, pas de police exotique, lisibles en clair
et en sombre (`prefers-color-scheme` via `<style>` inline).

### P4 — IDA\* et le 15-puzzle

**`ALGO_IDA` dans `src/search.h`.** Implémentation dans `src/search.c` :

```
seuil = h(racine)
boucle :
    (trouvé, seuil_suivant) = dfs(racine, g=0, seuil, coup_precedent=-1)
    si trouvé : rendre le chemin
    si seuil_suivant = +inf : pas de solution
    seuil = seuil_suivant
    iterations++
```

`dfs` est récursif, borné par `f = g + h > seuil`, et **n'a pas de table
d'états visités** : la seule élimination est l'anti-retour immédiat
(`move == puzzle_opposite(coup_precedent)`). C'est précisément ce qui donne la
mémoire en O(d) : la pile d'appel et le tableau de coups, rien d'autre.

Contraintes d'implémentation :

- La récursion travaille sur un unique plateau muté puis restauré (`puzzle_apply`
  supporte déjà `dst == src`, testé dans `tests/test_puzzle.c`), pas sur une copie
  par nœud.
- `stats_t` gagne `last_iter_generated` (nœuds générés par la dernière
  itération). Le facteur de ré-exploration est `generated / last_iter_generated` :
  c'est la réponse chiffrée à la question 5.
- `bytes_peak` = profondeur maximale atteinte × taille d'un cadre, plus le
  tableau de coups. Compté, pas estimé.
- `--max-nodes` est respecté et rend `SEARCH_ABORTED`.
- Le chemin rendu est la pile de coups de l'itération gagnante.

**Cible `taquin15` dans le Makefile** : mêmes sources, `-DWH_BOARD=4`, sortie
`bin/taquin15`. Idem `bin/test_search15` pour la suite de tests en 4x4.

**Génération d'instances 15-puzzle** : `--shuffle N` existe déjà et
`puzzle_solvable` gère le cas `WH_BOARD` pair (`src/puzzle.c:70`). Rien à
ajouter.

### P5 — L'écrit

**`README.md`, réécrit.** Plan imposé :

1. Ce que fait le projet, en trois phrases.
2. **Provenance** — tableau explicite. Décision prise avec l'auteur :
   `starting-kit/item.h` (16) + `starting-kit/list.h` (28) +
   `starting-kit/list.c` (272) + `src/board.h` (26) = **342 lignes fournies par
   l'école**. Tout le reste est écrit par l'auteur. Le tableau donne le
   décompte exact de chaque fichier, généré, pas recopié.
3. Comment compiler et lancer le banc.
4. Les résultats : les tableaux et les graphes, tirés des CSV.
5. Les six questions et leurs réponses (lien vers `docs/QUESTIONS.md`).
6. Lien vers la rétrospective.

**`docs/QUESTIONS.md`** — les six questions imposées, chacune répondue avec un
chiffre issu du banc quand il y en a un :

1. Admissibilité et consistance de Manhattan ; laquelle autorise à jeter un nœud
   déjà fermé.
2. Preuve que `+2` par conflit linéaire reste admissible, et pourquoi le retrait
   glouton est nécessaire.
3. Le cas où jeter un fils déjà sur l'open list sans comparer `g` est faux —
   avec le taux mesuré (2/120 déjà obtenu, à confirmer sur 2000 instances).
4. Complexité de `onList` avant et après ; ce qui est haché et comment les
   collisions sont traitées, avec la longueur de chaîne mesurée.
5. Facteur de ré-exploration d'IDA\*, mesuré, et pourquoi c'est quand même
   gagnant.
6. Uniformité du générateur : est-ce que 200 coups aléatoires depuis le but
   donnent une loi uniforme sur les états solvables, et comment on le vérifie.

**Réponse à la question 6 — vérification effective.** Le 3x3 a 9!/2 = 181 440
états solvables, ce qui est énumérable :

- `--enumerate` (3x3 uniquement) : BFS complet depuis l'état but sur tout
  l'espace, indexé par rang de permutation (code de Lehmer, tableau de 9! octets
  = 362 880 o). Produit la distribution exacte des profondeurs optimales.
- `--dist N` : tire N plateaux avec le générateur, lit leur profondeur optimale
  dans la table d'énumération (accès direct, instantané), et compare
  l'histogramme obtenu à la distribution exacte par un test du khi-deux.

La question n'est alors plus « comment le vérifierais-tu » mais « voici le
résultat ».

**`docs/RETROSPECTIVE.md`** — une page, trois sections :

- ce qui a été mesuré et ce que ça a montré ;
- ce qui a raté : le bug de `sift_down` (comparaison à une valeur périmée après
  la première descente, tas presque trié, A\* non optimal sur quelques
  instances seulement), le débordement potentiel de `RANDMAX()` du squelette
  (`neighbors[4]` accédé hors bornes quand le `float` arrondit à `4.0`), et
  l'assertion de dominance initialement trop forte dans les tests ;
- ce qui serait fait autrement : bases de données de motifs (pattern databases)
  plutôt que conflit linéaire, tas d'index plutôt que de pointeurs, et
  instrumentation dès le premier jour.

---

## Critères d'acceptation

1. `make test` passe, y compris les nouveaux tests 4x4 (`bin/test_search15`).
2. IDA\* rend **exactement** la même longueur que l'oracle BFS sur 120 instances
   3x3 tirées au hasard, pour les quatre heuristiques.
3. IDA\* rend exactement la même longueur qu'A\*/Manhattan sur 30 instances 4x4
   de mélange 40 (assez profondes pour être non triviales, assez peu pour
   qu'A\* tienne en mémoire).
4. Sur le 15-puzzle à mélange 200, IDA\*/Manhattan résout au moins 18 instances
   sur 20 avec une mémoire pic **inférieure à 64 Ko**, là où A\*/Manhattan
   dépasse 500 Mo ou abandonne.
5. `bench/run_bench.sh` s'exécute de bout en bout en moins de 10 minutes et
   produit les cinq CSV plus `ENVIRONMENT.txt`, sans intervention.
6. `bench/report.py` régénère `docs/BENCHMARKS.md` et les quatre SVG à
   l'identique à partir des CSV commités (idempotent, aucun aléa).
7. Chaque chiffre du README apparaît dans un CSV commité de `results/`. Aucun
   nombre écrit à la main.
8. `docs/QUESTIONS.md` répond aux six questions ; les questions 3, 4, 5 et 6
   citent un chiffre mesuré, pas un raisonnement seul.
9. `--enumerate` rend 181 440 états solvables et une profondeur maximale de 31
   (résultat connu du 8-puzzle : c'est le contrôle de correction de
   l'énumération).
10. `--dist 100000` rend une statistique du khi-deux et un verdict lisible sur
    l'uniformité du générateur.
11. Le tableau de provenance du README donne un décompte de lignes exact,
    vérifiable par `wc -l`, totalisant 342 lignes fournies.
12. Aucune régression : `bin/taquin`, `bin/taquin_vs`, `bin/taquin_baseline`
    compilent et fonctionnent comme avant.

---

## Plan de test

| Niveau | Quoi | Nouveaux |
|---|---|---|
| Unitaire | `ida_dfs` : seuil initial = h(racine) ; le seuil suivant est bien le min des f dépassant le seuil ; l'anti-retour n'élimine que le coup inverse | +3 |
| Unitaire | `--enumerate` : 181 440 états, profondeur max 31, histogramme sommant à 181 440 | +3 |
| Intégration | IDA\* contre l'oracle BFS, 120 instances 3x3, 4 heuristiques | +480 |
| Intégration | IDA\* contre A\*, 30 instances 4x4 (`bin/test_search15`) | +30 |
| Intégration | `--max-nodes` interrompt IDA\* et rend `SEARCH_ABORTED` | +1 |
| Intégration | le chemin rendu par IDA\* rejoué mène réellement au but | +150 |
| Bout en bout | `bench/run_bench.sh` puis `bench/report.py` : les SVG et le markdown se régénèrent à l'identique | +1 |

---

## Plan de retour arrière

Tout est additif : nouvelles valeurs d'enum, nouveaux fichiers, nouvelles cibles
de Makefile. Aucun chemin existant n'est modifié, hormis l'ajout d'un champ à
`stats_t` (qui élargit une colonne de CSV). Retour arrière = `git revert` du ou
des commits concernés ; `bin/taquin` continue de fonctionner sans IDA\*.

Les CSV de `results/` sont des données, pas du code : les régénérer suffit.

---

## Estimation d'effort

| Composant | Effort |
|---|---|
| IDA\* + compteur de ré-exploration + tests | 3 h |
| Cibles 4x4 (`taquin15`, `test_search15`) | 30 min |
| `--enumerate` / `--dist` + khi-deux + tests | 2 h |
| `bench/run_bench.sh` | 1 h |
| `bench/report.py` (tableaux + 4 SVG sans dépendance) | 3 h |
| Exécution du banc | 10 min |
| README + QUESTIONS.md + RETROSPECTIVE.md | 3 h |
| **Total** | **~12 h 40** |

---

## Fichiers concernés

| Fichier | Changement |
|---|---|
| `src/search.h` | `ALGO_IDA` dans `algo_id` |
| `src/search.c` | `solve_ida` + `ida_dfs` ; branchement dans `search_solve` |
| `src/stats.h` | champ `last_iter_generated` |
| `src/taquin.c` | colonne CSV, options `--enumerate` et `--dist N` |
| `src/enumerate.h` / `src/enumerate.c` | **nouveau** — rang de permutation, BFS complet, khi-deux |
| `Makefile` | cibles `taquin15`, `test_search15`, `bench` |
| `tests/test_search.c` | cas IDA\* |
| `tests/test_enumerate.c` | **nouveau** |
| `bench/run_bench.sh` | **nouveau** |
| `bench/report.py` | **nouveau** |
| `results/*.csv` | **nouveau** — données, commitées |
| `docs/BENCHMARKS.md`, `docs/img/*.svg` | **nouveau** — générés |
| `docs/QUESTIONS.md`, `docs/RETROSPECTIVE.md` | **nouveau** |
| `README.md` | réécrit |

---

## Hors périmètre

- **Bases de données de motifs (pattern databases).** C'est la suite logique
  après le conflit linéaire, et c'est un projet en soi (génération, stockage,
  compression). Mentionné dans la rétrospective comme « ce que je ferais
  ensuite », pas implémenté.
- **Le 24-puzzle.** Même avec IDA\*, il demande des bases de motifs pour être
  résolu en temps raisonnable.
- **Parallélisation.** Ni A\* parallèle, ni IDA\* réparti.
- **Le mode Joueur vs IA.** Il fonctionne, il n'est pas retouché.
- **Réécriture de l'historique Git** pour purger les PDF des anciens commits.
  Ils sont sortis du suivi ; les purger de l'historique est une opération
  destructrice qui demande un accord explicite et un `push --force`.

---

## Lié

- Phases 0 à 2 : commits `7a231e8`, `e449a00`, `824289d`, `9f70efd`, `fe7c9ce`,
  `6a211cb`, `0ca69f8` sur la branche `phases-0-5`.
