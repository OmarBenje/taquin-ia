#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "board.h"

/*
 * Taquin (sliding puzzle)
 * But : amener le plateau à l'état final  1 2 3
 *                                         4 5 6
 *                                         7 8 _
 * La case vide (blank) est représentée par 0.
 *
 * Ce fichier est l'adaptateur Item <-> plateau nu : les règles du jeu vivent
 * dans puzzle.c, on ne les réimplémente pas ici.
 */

/* Graine explicite : RANDINIT() ne vit plus dans initGame(). */
static rng_t g_rng = { 0 };

void boardSeed(uint64_t seed)
{
  rng_seed(&g_rng, seed);
}

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
  node->blank = (char)puzzle_blank((const cell_t *)board);
}

/* ------------------------------------------------------------------ */
/* printBoard : affichage grille du plateau                            */
/* ------------------------------------------------------------------ */
void printBoard(Item *node)
{
  assert(node);
  puzzle_print((const cell_t *)node->board);
}

/* ------------------------------------------------------------------ */
/* evaluateBoard : heuristique = distance de Manhattan totale          */
/* Retourne 0 si le plateau est dans l'état but.                      */
/* ------------------------------------------------------------------ */
double evaluateBoard(Item *node)
{
  int i, dist = 0;

  for (i = 0; i < MAX_BOARD; i++) {
    int val = (unsigned char)node->board[i];
    int goal_pos, dr, dc;

    if (val == 0) continue;            /* la case vide ne compte pas */

    goal_pos = val - 1;                /* position but de la tuile val */
    dr = i / WH_BOARD - goal_pos / WH_BOARD;
    dc = i % WH_BOARD - goal_pos % WH_BOARD;
    dist += (dr < 0 ? -dr : dr) + (dc < 0 ? -dc : dc);
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
  cell_t next[MAX_BOARD];
  Item  *child;
  int    new_blank;

  new_blank = puzzle_apply((const cell_t *)node->board,
                           (unsigned char)node->blank, move, next);
  if (new_blank < 0)
    return NULL;

  child = nodeAlloc();
  initBoard(child, (char *)next);

  child->parent = node;
  child->depth  = node->depth + 1;
  child->g      = node->g + 1;
  child->h      = (float)evaluateBoard(child);
  child->f      = child->g + child->h;

  return child;
}

/* ------------------------------------------------------------------ */
/* itemFromBoard : nœud racine à partir d'un plateau nu                */
/* ------------------------------------------------------------------ */
Item *itemFromBoard(const cell_t *board)
{
  Item *node = nodeAlloc();

  initBoard(node, (char *)board);
  node->depth  = 0;
  node->g      = 0;
  node->h      = (float)evaluateBoard(node);
  node->f      = node->h;
  node->parent = NULL;

  return node;
}

/* ------------------------------------------------------------------ */
/* initGame : génère un état initial aléatoire solvable                */
/* Principe : partir de l'état but et appliquer N mouvements aléatoires*/
/* La graine vient de boardSeed() — plus de RANDINIT() caché ici.      */
/* ------------------------------------------------------------------ */
Item *initGameShuffle(int nmoves)
{
  cell_t board[MAX_BOARD];

  puzzle_goal(board);
  puzzle_shuffle(board, nmoves, &g_rng);

  return itemFromBoard(board);
}

Item *initGame(void)
{
  return initGameShuffle(200);
}
