#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include "list.h"
#include "puzzle.h"

/*
 * Adaptateur entre la logique nue du taquin (puzzle.h) et la structure Item
 * du squelette fourni.  Les constantes WH_BOARD / MAX_BOARD / MAX_MOVES et les
 * codes MOVE_* viennent désormais de puzzle.h, pour que le solveur rapide et
 * le solveur historique partagent exactement les mêmes règles du jeu.
 */

/* Conservées pour la compatibilité avec le squelette ; le code de ce dépôt
 * ne les utilise plus (voir l'explication en tête de rng.h). */
#include <time.h>
#define RANDINIT()   srand(time(NULL))
#define RANDMAX(x)   (int)((float)(x) * rand() / (RAND_MAX + 1.0))

/*
 * Graine du générateur d'instances.  RANDINIT() a été sorti de initGame() :
 * c'est l'appelant qui décide de la graine, donc une exécution est rejouable
 * (`--seed N`).  Sans appel, la graine vaut 0.
 */
void   boardSeed(uint64_t seed);

Item  *initGame(void);                   /* mélange par défaut : 200 coups */
Item  *initGameShuffle(int nmoves);
Item  *itemFromBoard(const cell_t *board);

void   initBoard(Item *node, char *board);
Item  *getChildBoard(Item *node, int move);
double evaluateBoard(Item *node);
void   printBoard(Item *node);

#endif /* BOARD_H */
