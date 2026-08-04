# Taquin — Joueur vs IA

Projet IA en C implémentant le **jeu du Taquin** (8-puzzle) en mode interactif : le joueur affronte l'algorithme **A\*** et compare son score à la solution optimale.

---

## Présentation du jeu

Le Taquin est un puzzle classique composé d'une grille **3×3** contenant 8 tuiles numérotées et une case vide. Le but est de faire glisser les tuiles pour atteindre l'état final suivant :

```
+---+---+---+
| 1 | 2 | 3 |
+---+---+---+
| 4 | 5 | 6 |
+---+---+---+
| 7 | 8 |   |
+---+---+---+
```

À chaque coup, seule la tuile adjacente à la case vide peut être déplacée (haut, bas, gauche, droite).

---

## Mode Joueur vs IA

Lance `./bin/taquin_vs` pour affronter l'IA. Le programme :

1. Génère un puzzle aléatoire solvable
2. Calcule en silence la solution optimale avec A*
3. Te laisse jouer manuellement coup par coup
4. Affiche le score final et te dit de combien l'IA te devance (ou non !)

**Commandes en jeu :**

| Touche | Action |
|--------|--------|
| `z` | Déplacer la case vide vers le haut |
| `s` | Déplacer la case vide vers le bas |
| `q` | Déplacer la case vide vers la gauche |
| `d` | Déplacer la case vide vers la droite |
| `a` | Abandonner la partie |

La **distance de Manhattan restante** s'affiche après chaque coup pour t'aider à évaluer ta progression.

---

## L'algorithme A* (IA)

L'IA utilise A* avec l'heuristique de la **distance de Manhattan** :

```
f = g + h
```

| Terme | Signification |
|-------|--------------|
| `g`   | Coût réel — nombre de coups depuis l'état initial |
| `h`   | Heuristique — distance de Manhattan estimée jusqu'au but |
| `f`   | Coût total estimé |

Cette heuristique est **admissible** (ne surestime jamais le coût réel), ce qui garantit qu'A* trouve toujours la **solution optimale**.

---

## Structure du projet

```
starting-kit/      # squelette fourni par l'école, inchangé
├── item.h         # Structure de nœud (tuiles, coûts f/g/h, pointeurs parent/prev/next)
├── list.h         # Interface de la liste doublement chaînée
└── list.c         # Implémentation : addFirst, addLast, popFirst, popLast, popBest...
src/
├── puzzle.[ch]    # Règles du jeu sur un plateau nu
├── heuristic.[ch] # zero, misplaced, manhattan, manhattan + conflit linéaire
├── node.h node.c  # Nœud de recherche et allocateur par blocs
├── pqueue.[ch]    # Tas binaire (remplace popBest, O(log n) au lieu de O(n))
├── hashset.[ch]   # Table de hachage (remplace onList, O(1) au lieu de O(n))
├── search.[ch]    # BFS et A*, sur les listes du squelette ou sur les structures rapides
├── stats.[ch]     # Compteurs : nœuds générés, développés, mémoire pic, temps
├── board.[ch]     # Adaptateur vers la structure Item du squelette
├── taquin.c       # Banc de mesure  →  bin/taquin
├── taquin_vs.c    # Mode interactif Joueur vs IA  →  bin/taquin_vs
└── baseline.c     # Solveur d'origine restauré  →  bin/taquin_baseline
tests/             # make test
Makefile
```

---

## Compilation et exécution

### Prérequis

- GCC (ou Clang)
- `make`

### Compilation

```bash
make          # ~1 s
make test     # la suite de tests, ~3 s
```

### Exécution

```bash
./bin/taquin_vs                 # jouer contre l'IA
./bin/taquin_vs --seed 42       # rejouer exactement la même partie

./bin/taquin --runs 20 --verify # le banc de mesure : 20 instances, vérifiées
                                # contre l'oracle BFS
./bin/taquin --help             # toutes les options
```

---

## Auteur

**Omar Benjelloun** — Projet IA, 2026
