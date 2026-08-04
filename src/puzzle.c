#include <stdio.h>
#include <string.h>
#include "puzzle.h"

/* Largeur d'affichage d'une case : 2 caractères dès qu'on dépasse la tuile 9. */
#define CELLW ((MAX_BOARD - 1) >= 10 ? 2 : 1)

void puzzle_goal(cell_t *b)
{
  int i;
  for (i = 0; i < MAX_BOARD - 1; i++) b[i] = (cell_t)(i + 1);
  b[MAX_BOARD - 1] = 0;
}

int puzzle_is_goal(const cell_t *b)
{
  int i;
  for (i = 0; i < MAX_BOARD - 1; i++)
    if (b[i] != (cell_t)(i + 1)) return 0;
  return b[MAX_BOARD - 1] == 0;
}

int puzzle_blank(const cell_t *b)
{
  int i;
  for (i = 0; i < MAX_BOARD; i++)
    if (b[i] == 0) return i;
  return -1;
}

/*
 * Position d'arrivée de la case vide après `move`, ou -1 si le mouvement
 * sort du plateau.  Séparé de puzzle_apply pour pouvoir tester la légalité
 * d'un coup sans recopier le plateau (utile dans IDA*).
 */
static int blank_after(int blank, int move)
{
  switch (move) {
    case MOVE_UP:    return (blank >= WH_BOARD)                ? blank - WH_BOARD : -1;
    case MOVE_DOWN:  return (blank < MAX_BOARD - WH_BOARD)     ? blank + WH_BOARD : -1;
    case MOVE_LEFT:  return (blank % WH_BOARD != 0)            ? blank - 1        : -1;
    case MOVE_RIGHT: return (blank % WH_BOARD != WH_BOARD - 1) ? blank + 1        : -1;
    default:         return -1;
  }
}

int puzzle_apply(const cell_t *src, int blank, int move, cell_t *dst)
{
  int nb = blank_after(blank, move);
  if (nb < 0) return -1;

  if (dst != src) memcpy(dst, src, MAX_BOARD);
  dst[blank] = dst[nb];   /* la tuile voisine glisse dans le trou */
  dst[nb]    = 0;
  return nb;
}

/*
 * Solvabilité par parité des inversions.
 * Une inversion est une paire de tuiles (i < j en lecture ligne par ligne)
 * telle que la tuile en i porte un numéro plus grand que celle en j.
 * Un coup horizontal ne change aucune inversion ; un coup vertical en change
 * exactement WH_BOARD - 1.  Donc :
 *   - WH_BOARD impair : WH_BOARD - 1 est pair, la parité des inversions est
 *     invariante — solvable ssi elle est paire (celle de l'état but).
 *   - WH_BOARD pair : chaque coup vertical change la parité des inversions ET
 *     la ligne de la case vide — c'est la somme des deux qui est invariante.
 */
int puzzle_solvable(const cell_t *b)
{
  int i, j, inv = 0;

  for (i = 0; i < MAX_BOARD; i++) {
    if (b[i] == 0) continue;
    for (j = i + 1; j < MAX_BOARD; j++) {
      if (b[j] == 0) continue;
      if (b[i] > b[j]) inv++;
    }
  }

  if (WH_BOARD % 2 == 1)
    return (inv % 2) == 0;

  {
    int row_from_bottom = WH_BOARD - (puzzle_blank(b) / WH_BOARD);
    return ((inv + row_from_bottom) % 2) == 1;
  }
}

void puzzle_shuffle(cell_t *b, int nmoves, rng_t *rng)
{
  int blank = puzzle_blank(b);
  int forbidden = -1;   /* le coup qui annulerait le précédent */
  int k;

  for (k = 0; k < nmoves; k++) {
    int cand[MAX_MOVES], n = 0, m, i;

    for (m = 0; m < MAX_MOVES; m++) {
      if (m == forbidden) continue;
      if (blank_after(blank, m) >= 0) cand[n++] = m;
    }
    if (n == 0) break;   /* ne peut pas arriver sur un plateau >= 2x2 */

    i = (int)rng_below(rng, (unsigned)n);
    m = cand[i];
    blank = puzzle_apply(b, blank, m, b);
    forbidden = puzzle_opposite(m);
  }
}

void puzzle_print(const cell_t *b)
{
  int i, j;

  printf("\n");
  for (i = 0; i < WH_BOARD; i++) {
    for (j = 0; j < WH_BOARD; j++) printf("+%.*s", CELLW + 2, "-----");
    printf("+\n");

    for (j = 0; j < WH_BOARD; j++) {
      int val = b[i * WH_BOARD + j];
      if (val == 0) printf("| %*s ", CELLW, "");
      else          printf("| %*d ", CELLW, val);
    }
    printf("|\n");
  }
  for (j = 0; j < WH_BOARD; j++) printf("+%.*s", CELLW + 2, "-----");
  printf("+\n");
}

const char *puzzle_move_name(int move)
{
  switch (move) {
    case MOVE_UP:    return "haut";
    case MOVE_DOWN:  return "bas";
    case MOVE_LEFT:  return "gauche";
    case MOVE_RIGHT: return "droite";
    default:         return "?";
  }
}
