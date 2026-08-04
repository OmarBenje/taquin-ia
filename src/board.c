#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "board.h"

/*
 * Taquin (sliding puzzle)
 * But : amener le plateau à l'état final  1 2 3
 *                                         4 5 6
 *                                         7 8 _
 * La case vide (blank) est représentée par 0.
 */

/* ------------------------------------------------------------------ */
/* initBoard : copie un tableau de chars dans un nouveau nœud          */
/* ------------------------------------------------------------------ */
void initBoard(Item *node, char *board)
{
  assert(node);

  node->size  = MAX_BOARD;
  node->board = malloc(MAX_BOARD * sizeof(char));
  assert(node->board);
  memcpy(node->board, board, MAX_BOARD);

  /* retrouver la position de la case vide */
  for (int i = 0; i < MAX_BOARD; i++) {
    if (board[i] == 0) {
      node->blank = (char)i;
      break;
    }
  }
}

/* ------------------------------------------------------------------ */
/* printBoard : affichage grille du plateau                            */
/* ------------------------------------------------------------------ */
void printBoard(Item *node)
{
  assert(node);
  printf("\n");
  for (int i = 0; i < WH_BOARD; i++) {
    for (int j = 0; j < WH_BOARD; j++)
      printf("+---");
    printf("+\n");

    for (int j = 0; j < WH_BOARD; j++) {
      int val = (unsigned char)node->board[i * WH_BOARD + j];
      if (val == 0) printf("|   ");
      else          printf("| %d ", val);
    }
    printf("|\n");
  }
  for (int j = 0; j < WH_BOARD; j++)
    printf("+---");
  printf("+\n");
}

/* ------------------------------------------------------------------ */
/* evaluateBoard : heuristique = distance de Manhattan totale          */
/* Retourne 0 si le plateau est dans l'état but.                      */
/* ------------------------------------------------------------------ */
double evaluateBoard(Item *node)
{
  int dist = 0;
  for (int i = 0; i < MAX_BOARD; i++) {
    int val = (unsigned char)node->board[i];
    if (val == 0) continue;        /* la case vide ne compte pas */

    int goal_pos = val - 1;        /* position but de la tuile val */
    int cur_row  = i        / WH_BOARD;
    int cur_col  = i        % WH_BOARD;
    int goal_row = goal_pos / WH_BOARD;
    int goal_col = goal_pos % WH_BOARD;

    dist += abs(cur_row - goal_row) + abs(cur_col - goal_col);
  }
  return (double)dist;
}

/* ------------------------------------------------------------------ */
/* getChildBoard : génère l'état fils après avoir déplacé la case vide */
/* move : 0=haut  1=bas  2=gauche  3=droite                           */
/* Retourne NULL si le mouvement est invalide.                         */
/* ------------------------------------------------------------------ */
Item *getChildBoard(Item *node, int move)
{
  int blank     = (unsigned char)node->blank;
  int new_blank = -1;

  switch (move) {
    case MOVE_UP:    if (blank >= WH_BOARD)                   new_blank = blank - WH_BOARD; break;
    case MOVE_DOWN:  if (blank < MAX_BOARD - WH_BOARD)        new_blank = blank + WH_BOARD; break;
    case MOVE_LEFT:  if (blank % WH_BOARD != 0)               new_blank = blank - 1;        break;
    case MOVE_RIGHT: if (blank % WH_BOARD != WH_BOARD - 1)    new_blank = blank + 1;        break;
  }

  if (new_blank == -1)
    return NULL;

  Item *child = nodeAlloc();
  initBoard(child, node->board);

  /* échange la case vide avec la tuile voisine */
  child->board[blank]     = child->board[new_blank];
  child->board[new_blank] = 0;
  child->blank            = (char)new_blank;

  child->parent = node;
  child->depth  = node->depth + 1;
  child->g      = node->g + 1;
  child->h      = (float)evaluateBoard(child);
  child->f      = child->g + child->h;

  return child;
}

/* ------------------------------------------------------------------ */
/* initGame : génère un état initial aléatoire solvable                */
/* Principe : partir de l'état but et appliquer N mouvements aléatoires*/
/* ------------------------------------------------------------------ */
Item *initGame()
{
  RANDINIT();

  /* état but */
  char board[MAX_BOARD];
  for (int i = 0; i < MAX_BOARD - 1; i++) board[i] = (char)(i + 1);
  board[MAX_BOARD - 1] = 0;
  int blank      = MAX_BOARD - 1;
  int prev_blank = -1;

  /* mélange par 200 mouvements aléatoires (évite les aller-retours) */
  for (int k = 0; k < 200; k++) {
    int neighbors[4], n = 0;

    if (blank >= WH_BOARD)                  neighbors[n++] = blank - WH_BOARD;
    if (blank < MAX_BOARD - WH_BOARD)       neighbors[n++] = blank + WH_BOARD;
    if (blank % WH_BOARD != 0)              neighbors[n++] = blank - 1;
    if (blank % WH_BOARD != WH_BOARD - 1)   neighbors[n++] = blank + 1;

    int next;
    do { next = neighbors[RANDMAX(n)]; } while (next == prev_blank && n > 1);

    board[blank] = board[next];
    board[next]  = 0;
    prev_blank   = blank;
    blank        = next;
  }

  Item *node = nodeAlloc();
  initBoard(node, board);
  node->depth = 0;
  node->g     = 0;
  node->h     = (float)evaluateBoard(node);
  node->f     = node->h;

  return node;
}
