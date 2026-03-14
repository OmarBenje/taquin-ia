#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "board.h"

list_t openList;
list_t closedList;

/* ------------------------------------------------------------------ */
/* showSolution : affiche le chemin de l'état initial à l'état but     */
/* ------------------------------------------------------------------ */
void showSolution(Item *goal)
{
  /* collecter le chemin via les pointeurs parent */
  Item *path[10000];
  int   len = 0;
  Item *cur = goal;

  while (cur) {
    path[len++] = cur;
    cur = cur->parent;
  }

  printf("\n=== Solution (%d coups) ===\n", len - 1);
  for (int i = len - 1; i >= 0; i--) {
    printf("Etape %d :", len - 1 - i);
    printBoard(path[i]);
  }

  printf("Longueur de la solution : %d\n", len - 1);
  printf("Taille open list        : %d\n", openList.numElements);
  printf("Taille closed list      : %d\n", closedList.numElements);
}

/* ------------------------------------------------------------------ */
/* bfs : recherche en largeur (Breadth-First Search)                   */
/* Optimal en nombre de coups, mais lent (pas d'heuristique).          */
/* ------------------------------------------------------------------ */
void bfs(void)
{
  Item *cur_node, *child;

  while (listCount(&openList) != 0) {

    cur_node = popFirst(&openList);  /* FIFO */

    if (evaluateBoard(cur_node) == 0.0) {
      showSolution(cur_node);
      return;
    }

    addLast(&closedList, cur_node);  /* marquer comme visité */

    for (int move = 0; move < MAX_MOVES; move++) {
      child = getChildBoard(cur_node, move);
      if (child == NULL) continue;

      /* ignorer si déjà visité ou déjà dans l'open list */
      if (onList(&closedList, child->board) || onList(&openList, child->board)) {
        freeItem(child);
      } else {
        addLast(&openList, child);
      }
    }
  }

  printf("Aucune solution trouvée.\n");
}

/* ------------------------------------------------------------------ */
/* astar : algorithme A*                                               */
/* f = g + h  avec  g = coût réel (nb coups)                          */
/*                  h = distance de Manhattan (heuristique admissible) */
/* Optimal et bien plus rapide que BFS.                                */
/* ------------------------------------------------------------------ */
void astar(void)
{
  Item *cur_node, *child;

  while (listCount(&openList) != 0) {

    cur_node = popBest(&openList);   /* pop le nœud avec le plus petit f */

    if (evaluateBoard(cur_node) == 0.0) {
      showSolution(cur_node);
      return;
    }

    addLast(&closedList, cur_node);

    for (int move = 0; move < MAX_MOVES; move++) {
      child = getChildBoard(cur_node, move);
      if (child == NULL) continue;

      if (onList(&closedList, child->board) || onList(&openList, child->board)) {
        freeItem(child);
      } else {
        addLast(&openList, child);
      }
    }
  }

  printf("Aucune solution trouvée.\n");
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* Usage : ./taquin [bfs|astar]   (défaut : astar)                    */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
  int use_astar = 1;  /* A* par défaut */

  if (argc >= 2) {
    if (strcmp(argv[1], "bfs") == 0)   use_astar = 0;
    if (strcmp(argv[1], "astar") == 0) use_astar = 1;
  }

  initList(&openList);
  initList(&closedList);

  printf("=== Taquin %dx%d ===\n", WH_BOARD, WH_BOARD);
  printf("Algorithme : %s\n", use_astar ? "A* (Manhattan)" : "BFS");

  Item *initial = initGame();
  printf("\nEtat initial (h=%.0f) :", initial->h);
  printBoard(initial);

  addLast(&openList, initial);

  printf("\nRecherche en cours...\n");
  if (use_astar) astar();
  else           bfs();

  cleanupList(&openList);
  cleanupList(&closedList);

  return 0;
}
