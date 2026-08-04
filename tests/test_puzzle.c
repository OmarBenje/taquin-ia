#include <string.h>
#include "puzzle.h"
#include "tap.h"

int main(void)
{
  cell_t goal[MAX_BOARD], b[MAX_BOARD], c[MAX_BOARD], back[MAX_BOARD];
  rng_t rng;
  int i, m, blank, nb, nb2;

  puzzle_goal(goal);
  CHECK(puzzle_is_goal(goal), "l'etat but doit etre reconnu comme but");
  CHECK(puzzle_blank(goal) == MAX_BOARD - 1, "case vide en bas a droite");
  CHECK(puzzle_solvable(goal), "l'etat but est solvable");

  /* La case vide en coin n'a que deux mouvements legaux. */
  {
    int legal = 0;
    for (m = 0; m < MAX_MOVES; m++)
      if (puzzle_apply(goal, MAX_BOARD - 1, m, c) >= 0) legal++;
    CHECK(legal == 2, "coin bas-droit : 2 mouvements legaux, vu %d", legal);
  }

  /* Un mouvement suivi de son inverse ramene au plateau de depart. */
  rng_seed(&rng, 1234);
  for (i = 0; i < 2000; i++) {
    memcpy(b, goal, MAX_BOARD);
    puzzle_shuffle(b, (int)rng_below(&rng, 60), &rng);
    blank = puzzle_blank(b);

    for (m = 0; m < MAX_MOVES; m++) {
      nb = puzzle_apply(b, blank, m, c);
      if (nb < 0) continue;
      CHECK(c[nb] == 0, "la case vide doit atterrir en %d", nb);
      CHECK(puzzle_blank(c) == nb, "puzzle_blank doit suivre le mouvement");
      nb2 = puzzle_apply(c, nb, puzzle_opposite(m), back);
      CHECK(nb2 == blank, "le mouvement inverse ramene la case vide");
      CHECK(memcmp(back, b, MAX_BOARD) == 0, "aller-retour = identite");
    }
  }

  /* Toute marche aleatoire depuis le but reste dans la composante solvable. */
  for (i = 0; i < 5000; i++) {
    memcpy(b, goal, MAX_BOARD);
    puzzle_shuffle(b, (int)rng_below(&rng, 400), &rng);
    CHECK(puzzle_solvable(b), "un plateau melange depuis le but est solvable");
  }

  /* puzzle_apply doit accepter dst == src (utilise par puzzle_shuffle). */
  memcpy(b, goal, MAX_BOARD);
  memcpy(c, goal, MAX_BOARD);
  blank = MAX_BOARD - 1;
  nb  = puzzle_apply(b, blank, MOVE_UP, b);
  nb2 = puzzle_apply(c, blank, MOVE_UP, back);
  CHECK(nb == nb2 && memcmp(b, back, MAX_BOARD) == 0,
        "puzzle_apply doit supporter l'aliasing dst == src");

#if WH_BOARD == 3
  /* Echanger deux tuiles cree une inversion : le plateau devient insoluble. */
  {
    cell_t bad[9] = { 2, 1, 3, 4, 5, 6, 7, 8, 0 };
    CHECK(!puzzle_solvable(bad), "une seule transposition rend insoluble");
  }
#endif

  TAP_END("puzzle");
}
