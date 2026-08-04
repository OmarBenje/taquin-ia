# PR #2 — Phases 0 à 5 : instrumentation, structures de données, IDA\*, banc et écrit

- **PR** : https://github.com/OmarBenje/taquin-ia/pull/2
- **Issue** : [#1](https://github.com/OmarBenje/taquin-ia/issues/1) (`Closes #1`)
- **Branche** : `phases-0-5` → `main`
- **État** : **fusionnée** dans `main` (`0804c2c`), issue #1 fermée
- **Commits** : 18, chacun compile ; tous ceux postérieurs à l'ajout de la
  cible `test` la passent
- **CI** : verte sur `main`, Ubuntu et macOS

---

## Contexte

Le dépôt était un TP d'école : un solveur BFS/A\* sur les listes chaînées
fournies, un mode interactif, et un README qui décrivait un mini-jeu console.
Aucune mesure, aucun test, et un solveur BFS supprimé en cours de route — donc
plus rien pour prouver que l'A\* rendait bien l'optimum.

L'objectif de cette PR était de le transformer en pièce défendable en entretien :
instrumenter avant d'optimiser, refaire les structures de données, ajouter IDA\*
pour survivre au 15-puzzle, et publier des chiffres traçables plutôt que des
affirmations.

---

## Ce qui a changé

### Phase 0 — reproductibilité

| fichier | changement |
|---|---|
| `.gitignore` | binaires, `*.pdf`, `.DS_Store` |
| `IAEtJeux2.pdf`, `TP1-2.pdf`, `starting-kit/list.o`, `list_test`, `taquin_vs` | sortis du suivi (2,5 Mo) |
| `src/rng.h` | **nouveau** — splitmix64, tirage portable |
| `src/puzzle.[ch]` | **nouveau** — règles du jeu sur plateau nu, `WH_BOARD` surchargeable |
| `src/board.[ch]` | devient un adaptateur vers `Item` ; `RANDINIT()` sort d'`initGame` |
| `src/baseline.c` | **restauré** depuis `11f4e90` — le solveur supprimé, comme oracle |

`--seed N` rejoue exactement la même instance, sur n'importe quelle machine.

### Phase 1 — instrumenter avant d'optimiser

`src/stats.[ch]`, `src/search.[ch]`, `src/taquin.c` : compteurs (nœuds générés,
développés, open max, mémoire pic, temps) et un banc qui écrit une ligne de CSV
par instance.

### Phase 2 — structures de données

| fichier | remplace | complexité |
|---|---|---|
| `src/pqueue.[ch]` | `popBest` | O(n) → O(log n) |
| `src/hashset.[ch]` | `onList` | O(n) + `memcmp` par nœud → O(1) |
| `src/node.[ch]` | `malloc` par nœud | allocateur par blocs |

### Phase 3 — heuristiques

`src/heuristic.[ch]` : `zero`, `misplaced`, `manhattan`, `linear` (conflit
linéaire avec retrait glouton, admissible).

### Phase 4 — IDA\* et le 15-puzzle

`solve_ida` dans `src/search.c`, cibles `taquin15` et `test_search15`, option
`--max-bytes`.

### Phase 5 — l'écrit

`README.md` réécrit, `docs/QUESTIONS.md`, `docs/DESIGN.md`,
`docs/RETROSPECTIVE.md`, `docs/BENCHMARKS.md` (généré), `docs/img/*.png`,
`bench/run_bench.sh`, `bench/report.py`, `.github/workflows/ci.yml`.

---

## Vérification

| contrôle | résultat |
|---|---|
| `make all` | **OK**, zéro avertissement (`-Wall -Wextra -O2`) |
| `make test` | **OK** — 596 000+ vérifications, 0 échec |
| Chaque commit isolé (`git worktree`) | **16/16 compilent**, tests verts |
| `make bench` | **OK**, 3 min 32, 8 545 lignes de CSV |
| `make report` | **OK**, idempotent |
| Chiffres du README recoupés contre les CSV | **OK** |
| CI (Ubuntu + macOS) | **verte**, après correction de deux bugs qu'elle a trouvés |

### Ce que la CI a trouvé à son premier passage

Elle a échoué immédiatement sur Ubuntu, sur deux problèmes qu'aucune exécution
locale ne pouvait révéler :

- `-std=c11` définit `__STRICT_ANSI__`, ce qui masque `CLOCK_MONOTONIC` et
  `getrusage` sur glibc. **Le projet ne compilait pas sur Linux.** Corrigé en
  demandant explicitement `_POSIX_C_SOURCE` et `_DEFAULT_SOURCE` — mais
  seulement hors macOS, où `_POSIX_C_SOURCE` fait au contraire disparaître
  `ru_maxrss`.
- `print_verdict(…, chi2_fit(…, &df, …), df)` lisait `df` dans le même appel qui
  l'écrivait. L'ordre d'évaluation des arguments n'est pas spécifié en C : le
  compilateur avait le droit de lire `df` avant l'appel. GCC l'a signalé, clang
  non.

### Mesures publiées

| | avant | après |
|---|---|---|
| débit A\* | 122 428 nœuds/s | 20 359 496 nœuds/s (166×) |
| nœuds A\* contre BFS | — | 176× moins, solution identique |
| 15-puzzle, mélange 200 | A\* 5/10, 123 Mo | IDA\* 10/10, 768 octets |
| solutions non optimales | 116/1000 | 0/1000 |

---

## Bugs trouvés et corrigés en cours de route

1. **`sift_down` comparait à une valeur périmée** après la première descente. Le
   tas restait *presque* trié, les `f` sortaient dans le bon ordre la plupart du
   temps, et A\* rendait une solution *presque* optimale sur quelques instances.
   Attrapé par la vérification d'invariant du tas après chaque opération, pas
   par le contrôle d'ordre de sortie.
2. **Le README donnait `./taquin_vs`**, qui n'existait plus depuis la
   restructuration. Première commande tapée par un lecteur, cassée pendant neuf
   commits.
3. **Le Makefile ne recompilait rien** quand un en-tête ou `CFLAGS` changeaient.
4. **`--runs abc` et `--seed lol`** étaient acceptés en silence (`atoi` rend 0).
5. **Le branchement d'IDA\* était prévu à l'intérieur d'un `switch`**, avant la
   première étiquette `case` : code mort, sans avertissement. IDA\* aurait
   exécuté A\* en silence en écrivant `algo=ida` dans le CSV.
6. **Deux dépassements latents** dans le module d'énumération (file dimensionnée
   au bit près, `obs_depth[255]` hors bornes).
7. **Une prédiction fausse dans ma propre spec** : la loi du générateur était
   annoncée à 8,33 / 12,50 / 16,67 %. La mesure donne 16,67 / 0 / 33,33 %, parce
   que le graphe est biparti.

---

## Points de suivi

- **Bases de motifs (pattern databases)** — hors périmètre assumé, mais c'est la
  suite logique : elles rendraient résolubles en secondes les instances qu'IDA\*
  met une minute à traiter.
- **Manhattan incrémental** — le delta d'un coup est en O(1), je recalcule la
  distance entière sur chaque fils. Facteur 2 à 3 laissé sur la table, et c'est
  le débit qui limite IDA\* sur le 15-puzzle, pas la mémoire.
- **Historique Git** — fait après la fusion (`c7b0eb2`) : `git filter-repo` a
  retiré les PDF, les binaires et `docs/superpowers/` des 27 commits, et
  l'historique réécrit a été poussé en force. Un clone frais passe de 3,5 Mo à
  796 Ko. **Réserve** : `refs/pull/2/head` conserve l'ancienne pointe de branche
  côté GitHub, donc les anciens objets restent accessibles à qui a accès au
  dépôt via `git fetch origin refs/pull/2/head`. Seul le support GitHub peut les
  faire disparaître ; le dépôt étant privé, le risque est limité.
- **Provenance** — la répartition entre le squelette de l'école et le code du TP
  est déclarée, non déduite : le premier commit du dépôt contient déjà le projet
  terminé.
