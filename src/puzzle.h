#ifndef PUZZLE_H
#define PUZZLE_H

#include "rng.h"

/*
 * Représentation « nue » du taquin : un simple tableau de MAX_BOARD octets,
 * sans nœud ni liste.  Toute la logique du jeu vit ici ; board.c n'est plus
 * qu'un adaptateur vers la structure Item du squelette fourni.
 *
 * WH_BOARD est surchargeable à la compilation :  -DWH_BOARD=4  produit le
 * binaire 15-puzzle (voir la cible `taquin15` du Makefile).
 */

#ifndef WH_BOARD
#define WH_BOARD 3
#endif

#define MAX_BOARD (WH_BOARD * WH_BOARD)
#define MAX_MOVES 4

/* Déplacements de la case vide. */
#define MOVE_UP    0
#define MOVE_DOWN  1
#define MOVE_LEFT  2
#define MOVE_RIGHT 3

typedef unsigned char cell_t;

/* État but : 1 2 3 / 4 5 6 / 7 8 _   (la case vide vaut 0 et finit en bas à droite) */
void puzzle_goal(cell_t *b);
int  puzzle_is_goal(const cell_t *b);
int  puzzle_blank(const cell_t *b);

/*
 * Applique `move` à `src` (dont la case vide est en `blank`) et écrit le
 * résultat dans `dst`.  Retourne la nouvelle position de la case vide, ou -1
 * si le mouvement sort du plateau.  `dst` n'est pas modifié dans ce cas.
 */
int  puzzle_apply(const cell_t *src, int blank, int move, cell_t *dst);

/* Mouvement inverse : UP<->DOWN, LEFT<->RIGHT. */
static inline int puzzle_opposite(int move) { return move ^ 1; }

/* Test de solvabilité par parité des inversions (voir README, question 6). */
int  puzzle_solvable(const cell_t *b);

/* Mélange par marche aléatoire depuis l'état courant, sans retour immédiat. */
void puzzle_shuffle(cell_t *b, int nmoves, rng_t *rng);

void puzzle_print(const cell_t *b);
const char *puzzle_move_name(int move);

#endif /* PUZZLE_H */
