# Taquin — Résolution par BFS et A*

Projet IA en C implémentant le **jeu du Taquin** (8-puzzle) résolu automatiquement par deux algorithmes de recherche : **BFS** (Breadth-First Search) et **A\*** avec l'heuristique de la distance de Manhattan.

---

## Présentation du jeu

Le Taquin est un puzzle classique composé d'une grille **3×3** contenant 8 tuiles numérotées et une case vide. Le joueur (ici l'IA) doit faire glisser les tuiles pour atteindre l'état final suivant :

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

## Algorithmes implémentés

### BFS — Breadth-First Search

La recherche en largeur explore les états niveau par niveau (FIFO). Elle **garantit la solution optimale** en nombre de coups, mais explore un grand nombre d'états inutiles car elle n'utilise aucune heuristique.

- Complexité temporelle : **O(b^d)** avec b = facteur de branchement, d = profondeur
- Mémoire : élevée (tous les états du niveau courant sont gardés en mémoire)
- Adapté aux instances **faciles** (peu de coups nécessaires)

### A* — A-Star

A* est un algorithme de recherche informée qui sélectionne à chaque itération le nœud avec le **score f minimal** :

```
f = g + h
```

| Terme | Signification |
|-------|--------------|
| `g`   | Coût réel — nombre de coups depuis l'état initial |
| `h`   | Heuristique — distance de Manhattan estimée jusqu'au but |
| `f`   | Coût total estimé du chemin passant par ce nœud |

#### Distance de Manhattan

Pour chaque tuile, on calcule la distance entre sa position actuelle et sa position dans l'état final :

```
h = Σ |ligne_actuelle - ligne_but| + |colonne_actuelle - colonne_but|
```

Cette heuristique est **admissible** (ne surestime jamais le coût réel), ce qui garantit qu'A* trouve toujours la solution optimale.

**Exemple :**
```
+---+---+---+       Tuile 8 est en (0,0), son but est (2,1)
| 8 | 3 | 7 |   →  distance = |0-2| + |0-1| = 3
| 5 |   | 2 |       ...
| 6 | 1 | 4 |       h total = somme pour toutes les tuiles
+---+---+---+
```

A* est **significativement plus rapide** que BFS pour les instances difficiles grâce à cette heuristique.

---

## Structure du projet

```
starting-kit/
├── item.h        # Structure de nœud (tuiles, coûts f/g/h, pointeurs parent/prev/next)
├── list.h        # Interface de la liste doublement chaînée
├── list.c        # Implémentation : addFirst, addLast, popFirst, popLast, popBest, delList...
├── board.h       # Constantes du jeu (WH_BOARD, MAX_BOARD, MAX_MOVES) et prototypes
├── board.c       # Logique du Taquin : initGame, printBoard, evaluateBoard, getChildBoard
├── taquin.c      # Algorithmes BFS et A*, affichage de la solution, main
├── nqueens.c     # Squelette fourni par le professeur (problème des N-Reines, BFS)
└── Makefile
```

### `item.h` — Structure d'un nœud de recherche

```c
typedef struct Item_s {
  char  size;          // taille du plateau
  char *board;         // tableau des tuiles (0 = case vide)
  char  blank;         // position de la case vide
  float f, g, h;       // coûts : total, réel, heuristique
  int   depth;         // profondeur dans l'arbre de recherche
  struct Item_s *parent;     // nœud parent (pour reconstruire le chemin)
  struct Item_s *prev, *next; // chaînage dans la liste ouverte/fermée
} Item;
```

### `list.h/list.c` — Liste doublement chaînée

Utilisée comme **open list** et **closed list** pour les algorithmes de recherche.

| Fonction | Description |
|----------|-------------|
| `initList(list)` | Initialise une liste vide |
| `addFirst(list, node)` | Insère un nœud en tête |
| `addLast(list, node)` | Insère un nœud en queue |
| `popFirst(list)` | Retire et retourne le premier nœud (BFS) |
| `popLast(list)` | Retire et retourne le dernier nœud |
| `popBest(list)` | Retire et retourne le nœud avec le `f` minimal (A*) |
| `delList(list, node)` | Supprime un nœud quelconque |
| `onList(list, board)` | Recherche un état dans la liste (comparaison par `memcmp`) |
| `cleanupList(list)` | Libère tous les nœuds de la liste |

### `board.c` — Logique du Taquin

| Fonction | Description |
|----------|-------------|
| `initGame()` | Génère un état initial **solvable** par 200 mouvements aléatoires depuis l'état but |
| `initBoard(node, board)` | Copie un plateau dans un nœud, détecte la position de la case vide |
| `printBoard(node)` | Affiche la grille du plateau |
| `evaluateBoard(node)` | Calcule la distance de Manhattan (retourne 0 si résolu) |
| `getChildBoard(node, move)` | Génère l'état fils après un mouvement (0=haut, 1=bas, 2=gauche, 3=droite) |

### `taquin.c` — Algorithmes de recherche

| Fonction | Description |
|----------|-------------|
| `bfs()` | Recherche en largeur — utilise `popFirst` (FIFO) |
| `astar()` | Recherche A* — utilise `popBest` (min f) |
| `showSolution(goal)` | Reconstruit et affiche le chemin de l'état initial au but |
| `main(argc, argv)` | Point d'entrée — choisit l'algorithme selon l'argument passé |

---

## Compilation et exécution

### Prérequis

- GCC (ou Clang)
- `make`

### Compilation

```bash
make
```

### Exécution

```bash
./taquin astar   # résolution par A* (recommandé, par défaut)
./taquin bfs     # résolution par BFS
```

### Exemple de sortie

```
=== Taquin 3x3 ===
Algorithme : A* (Manhattan)

Etat initial (h=12) :
+---+---+---+
| 1 | 2 | 3 |
+---+---+---+
| 4 |   | 5 |
+---+---+---+
| 7 | 8 | 6 |
+---+---+---+

Recherche en cours...

=== Solution (4 coups) ===
Etape 0 : ...
Etape 1 : ...
...
Longueur de la solution : 4
Taille open list        : 12
Taille closed list      : 8
```

---

## Comparaison BFS vs A*

| Critère | BFS | A* |
|---------|-----|----|
| Heuristique | Aucune | Distance de Manhattan |
| Optimalité | Oui | Oui |
| Vitesse | Lente | Rapide |
| Mémoire | Élevée | Modérée |
| Cas d'usage | Instances simples | Toutes instances |

Pour un puzzle à **26 coups** (instance difficile), A* explore environ **3 700 nœuds** là où BFS en explore plusieurs centaines de milliers.

---

## Concepts clés

- **Open list** : nœuds découverts mais pas encore explorés
- **Closed list** : nœuds déjà explorés (évite les cycles)
- **État solvable** : un Taquin 3×3 est solvable si et seulement si le nombre d'inversions dans le tableau est pair
- **Admissibilité** : une heuristique est admissible si elle ne surestime jamais le coût réel → garantit l'optimalité de A*

---

## Auteur

**Omar Benjelloun** — Projet IA, 2026
