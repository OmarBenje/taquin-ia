#ifndef BOARD_H
#define BOARD_H

#include "list.h"
#include <time.h>

#define RANDINIT()   srand(time(NULL))
#define RANDMAX(x)   (int)((float)(x) * rand() / (RAND_MAX + 1.0))

#define WH_BOARD   3                      /* largeur/hauteur du plateau  */
#define MAX_BOARD  (WH_BOARD * WH_BOARD)  /* nombre de cases             */
#define MAX_MOVES  4                      /* haut, bas, gauche, droite   */

/* move codes */
#define MOVE_UP    0
#define MOVE_DOWN  1
#define MOVE_LEFT  2
#define MOVE_RIGHT 3

Item  *initGame();
void   initBoard(Item *node, char *board);
Item  *getChildBoard(Item *node, int move);
double evaluateBoard(Item *node);
void   printBoard(Item *node);

#endif
