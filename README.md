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

Lance `./taquin_vs` pour affronter l'IA. Le programme :

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
starting-kit/
├── item.h        # Structure de nœud (tuiles, coûts f/g/h, pointeurs parent/prev/next)
├── list.h        # Interface de la liste doublement chaînée
├── list.c        # Implémentation : addFirst, addLast, popFirst, popLast, popBest...
├── board.h       # Constantes du jeu et prototypes
├── board.c       # Logique du Taquin : initGame, printBoard, evaluateBoard, getChildBoard
├── taquin_vs.c   # Mode interactif Joueur vs IA
└── Makefile
```

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
./taquin_vs
```

---

## Auteur

**Omar Benjelloun** — Projet IA, 2026
