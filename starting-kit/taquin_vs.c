#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "board.h"

/* ------------------------------------------------------------------ */
/* astar_count : exécute A* en silence, retourne le nb optimal de     */
/* coups, ou -1 si aucune solution.                                    */
/* ------------------------------------------------------------------ */
static int astar_count(char *initial_board)
{
  list_t openList, closedList;
  initList(&openList);
  initList(&closedList);

  Item *start = nodeAlloc();
  initBoard(start, initial_board);
  start->depth  = 0;
  start->g      = 0;
  start->h      = (float)evaluateBoard(start);
  start->f      = start->h;
  start->parent = NULL;
  addLast(&openList, start);

  int result = -1;

  while (listCount(&openList) != 0) {
    Item *cur = popBest(&openList);

    if (evaluateBoard(cur) == 0.0) {
      result = (int)cur->g;
      addLast(&closedList, cur);
      break;
    }

    addLast(&closedList, cur);

    for (int move = 0; move < MAX_MOVES; move++) {
      Item *child = getChildBoard(cur, move);
      if (child == NULL) continue;

      if (onList(&closedList, child->board) || onList(&openList, child->board))
        freeItem(child);
      else
        addLast(&openList, child);
    }
  }

  cleanupList(&openList);
  cleanupList(&closedList);
  return result;
}

/* ------------------------------------------------------------------ */
/* print_controls                                                      */
/* ------------------------------------------------------------------ */
static void print_controls(void)
{
  printf("Commandes : z=haut  s=bas  q=gauche  d=droite  a=abandonner\n");
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
  printf("╔══════════════════════════════════╗\n");
  printf("║   TAQUIN  —  Joueur  vs  IA      ║\n");
  printf("╚══════════════════════════════════╝\n\n");

  /* Générer l'état initial */
  Item *initial = initGame();

  /* Sauvegarder le plateau pour lancer A* séparément */
  char saved_board[MAX_BOARD];
  memcpy(saved_board, initial->board, MAX_BOARD);
  freeItem(initial);

  /* A* calcule la solution optimale (en arrière-plan) */
  printf("L'IA calcule la solution optimale...\n");
  int ai_moves = astar_count(saved_board);
  if (ai_moves < 0) {
    printf("Erreur : aucune solution trouvée.\n");
    return 1;
  }
  printf("Prêt !  (La solution optimale est de %d coups — vous le saurez à la fin)\n\n", ai_moves);

  /* Initialiser l'état courant du joueur */
  Item *current = nodeAlloc();
  initBoard(current, saved_board);
  current->depth  = 0;
  current->g      = 0;
  current->h      = (float)evaluateBoard(current);
  current->f      = current->h;
  current->parent = NULL;

  int player_moves = 0;
  char input[32];

  printf("État initial :");
  printBoard(current);
  printf("\nObjectif : amener le plateau à  1 2 3 / 4 5 6 / 7 8 _\n\n");
  print_controls();

  /* ---- boucle de jeu ---- */
  while (1) {
    printf("\n[Coup %d | Manhattan restante : %.0f]  > ",
           player_moves + 1, evaluateBoard(current));
    fflush(stdout);

    if (fgets(input, sizeof(input), stdin) == NULL)
      break;

    input[strcspn(input, "\n")] = '\0';  /* supprimer \n */
    if (strlen(input) == 0) continue;

    char cmd  = input[0];
    int  move = -1;

    if      (cmd == 'z' || cmd == 'Z') move = MOVE_UP;
    else if (cmd == 's' || cmd == 'S') move = MOVE_DOWN;
    else if (cmd == 'q' || cmd == 'Q') move = MOVE_LEFT;
    else if (cmd == 'd' || cmd == 'D') move = MOVE_RIGHT;
    else if (cmd == 'a' || cmd == 'A') {
      printf("\n--- Abandon ---\n");
      printf("Vos coups : %d\n", player_moves);
      printf("IA (A*)   : %d coups (solution optimale)\n", ai_moves);
      freeItem(current);
      return 0;
    } else {
      printf("Commande inconnue. ");
      print_controls();
      continue;
    }

    /* Tenter le mouvement */
    Item *next = getChildBoard(current, move);
    if (next == NULL) {
      printf("Mouvement impossible dans cette direction.\n");
      continue;
    }

    /* next->parent pointe sur current — on détache avant de libérer */
    next->parent = NULL;
    Item *old = current;
    current    = next;
    freeItem(old);
    player_moves++;

    printBoard(current);

    /* Victoire ? */
    if (evaluateBoard(current) == 0.0) {
      printf("\n★ Bravo ! Puzzle résolu en %d coups ! ★\n\n", player_moves);
      printf("══════════════ Résultats ══════════════\n");
      printf("  Joueur : %d coups\n", player_moves);
      printf("  IA (A*): %d coups  (solution optimale)\n", ai_moves);
      printf("  Écart  : %+d coup(s)\n", player_moves - ai_moves);
      if (player_moves == ai_moves)
        printf("  Parfait ! Vous avez trouvé la solution optimale !\n");
      else if (player_moves < ai_moves)
        printf("  Incroyable, vous battez l'IA !\n");
      else
        printf("  L'IA vous devance de %d coup(s).\n", player_moves - ai_moves);
      printf("═══════════════════════════════════════\n");
      freeItem(current);
      return 0;
    }
  }

  freeItem(current);
  return 0;
}
