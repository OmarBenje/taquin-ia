#include <stdlib.h>
#include <string.h>
#include "enumerate.h"
#include "tap.h"

int main(void)
{
#if WH_BOARD != 3
  printf("%-18s (ignore : enumeration reservee au 3x3)\n", "enumeration");
  return 0;
#else
  unsigned char *d;
  long   n = 0, r;
  int    maxd = 0, k;
  cell_t b[MAX_BOARD], back[MAX_BOARD];

  /*
   * Bijectivite de perm_rank, prouvee et non echantillonnee.
   *
   * Un test qui tire des plateaux au hasard et note « ce rang a ete vu » sans
   * jamais relire la note ne teste rien du tout. On fait l'aller-retour sur
   * les 362 880 permutations : rank(unrank(r)) == r pour tout r prouve
   * l'injectivite, dont depend la surete memoire de enum_depths.
   */
  {
    int bad = 0;
    for (r = 0; r < 362880L; r++) {
      perm_unrank(r, b);
      if (perm_rank(b) != r) bad++;
    }
    CHECK(bad == 0, "perm_rank o perm_unrank != identite sur %d rangs", bad);
  }

  /* Les tuiles doivent former une permutation de 0..8, pas n'importe quoi. */
  {
    int seen[MAX_BOARD] = { 0 }, bad = 0;
    perm_unrank(123456, b);
    for (k = 0; k < MAX_BOARD; k++) {
      if (b[k] >= MAX_BOARD || seen[b[k]]) bad++;
      else seen[b[k]] = 1;
    }
    CHECK(bad == 0, "perm_unrank doit rendre une permutation");
  }

  /*
   * L'espace atteignable depuis le but fait exactement 9!/2 etats et le pire
   * cas du 8-puzzle est a 31 coups. Ces deux nombres sont connus : ils
   * valident l'enumeration elle-meme, pas seulement sa coherence interne.
   */
  d = enum_depths(&n, &maxd);
  CHECK(d != NULL, "enum_depths doit reussir");
  CHECK(n == 181440, "9!/2 = 181440 etats solvables, vu %ld", n);
  CHECK(maxd == 31, "le pire cas du 8-puzzle est a 31 coups, vu %d", maxd);

  puzzle_goal(b);
  CHECK(d[perm_rank(b)] == 0, "le but est a distance 0 de lui-meme");

  {
    cell_t bad_board[9] = { 2, 1, 3, 4, 5, 6, 7, 8, 0 };
    CHECK(d[perm_rank(bad_board)] == ENUM_UNREACHABLE,
          "un etat insoluble doit rester inatteignable");
  }

  /*
   * La parite : c'est le fait central que le generateur revele.
   * La profondeur optimale d'un etat a la meme parite que le nombre de coups
   * qui y mene depuis le but, donc un melange de N coups ne peut atteindre
   * que des profondeurs de parite N.
   */
  {
    rng_t rng;
    int   bad = 0, i;

    rng_seed(&rng, 777);
    for (i = 0; i < 3000; i++) {
      int sh = (int)rng_below(&rng, 100) * 2;        /* toujours pair */
      puzzle_goal(back);
      puzzle_shuffle(back, sh, &rng);
      if ((d[perm_rank(back)] & 1) != 0) bad++;
    }
    CHECK(bad == 0, "un melange pair ne peut donner qu'une profondeur paire "
                    "(%d contre-exemples)", bad);

    bad = 0;
    for (i = 0; i < 3000; i++) {
      int sh = (int)rng_below(&rng, 100) * 2 + 1;    /* toujours impair */
      puzzle_goal(back);
      puzzle_shuffle(back, sh, &rng);
      if ((d[perm_rank(back)] & 1) != 1) bad++;
    }
    CHECK(bad == 0, "un melange impair ne peut donner qu'une profondeur "
                    "impaire (%d contre-exemples)", bad);
  }

  free(d);
  TAP_END("enumeration");
#endif
}
