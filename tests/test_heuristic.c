#include <string.h>
#include "heuristic.h"
#include "tap.h"

int main(void)
{
  cell_t goal[MAX_BOARD], b[MAX_BOARD], child[MAX_BOARD];
  rng_t rng;
  heuristic_id id;
  int i, m, blank, nb;

  puzzle_goal(goal);

  /* h(but) = 0 pour toutes : condition necessaire a l'admissibilite. */
  for (id = 0; id < H_COUNT; id++)
    CHECK(heuristic_eval(id, goal) == 0,
          "%s doit valoir 0 sur l'etat but", heuristic_name(id));

  CHECK(heuristic_parse("linear", &id) == 0 && id == H_LINEAR, "parse linear");
  CHECK(heuristic_parse("nawak", &id) != 0, "parse d'un nom inconnu doit echouer");

  rng_seed(&rng, 20260804);
  for (i = 0; i < 30000; i++) {
    int hz, hm, hd, hl;

    memcpy(b, goal, MAX_BOARD);
    puzzle_shuffle(b, (int)rng_below(&rng, 300), &rng);

    hz = heuristic_eval(H_ZERO, b);
    hd = heuristic_eval(H_MISPLACED, b);
    hm = heuristic_eval(H_MANHATTAN, b);
    hl = heuristic_eval(H_LINEAR, b);

    /* Dominance : chaque heuristique est au moins aussi informee que la
       precedente. C'est ce qui garantit qu'elle developpe moins de noeuds. */
    CHECK(hz <= hd && hd <= hm && hm <= hl,
          "dominance violee : zero=%d misplaced=%d manhattan=%d linear=%d",
          hz, hd, hm, hl);

    /* Le surcout des conflits lineaires est toujours pair (+2 par conflit). */
    CHECK((hl - hm) % 2 == 0, "surcout de conflit lineaire impair : %d", hl - hm);

    /* Consistance : h(n) <= cout(n, n') + h(n') avec cout = 1 pour tout coup.
       C'est ce qui autorise a ignorer definitivement un noeud deja ferme. */
    blank = puzzle_blank(b);
    for (m = 0; m < MAX_MOVES; m++) {
      nb = puzzle_apply(b, blank, m, child);
      if (nb < 0) continue;
      for (id = 0; id < H_COUNT; id++) {
        int h  = heuristic_eval(id, b);
        int hc = heuristic_eval(id, child);
        CHECK(h <= hc + 1, "%s non consistante : h=%d h(fils)=%d",
              heuristic_name(id), h, hc);
      }
    }
  }

#if WH_BOARD == 3
  /* Cas d'ecole du conflit lineaire : les tuiles 1 et 2 sont toutes deux dans
     leur ligne but mais inversees. Manhattan compte 2, or il faut au moins 2
     coups de plus pour que l'une sorte de la ligne et y revienne. */
  {
    cell_t conflit[9] = { 2, 1, 3, 4, 5, 6, 7, 8, 0 };
    CHECK(heuristic_eval(H_MANHATTAN, conflit) == 2, "manhattan = 2");
    CHECK(heuristic_eval(H_LINEAR, conflit) == 4, "linear = 2 + 2 = 4");
  }
  /* Trois tuiles mutuellement en conflit forment 3 paires, mais faire sortir
     deux tuiles suffit : +4, pas +6. C'est tout l'objet du retrait glouton. */
  {
    cell_t triple[9] = { 3, 2, 1, 4, 5, 6, 7, 8, 0 };
    CHECK(heuristic_eval(H_MANHATTAN, triple) == 4, "manhattan = 4");
    CHECK(heuristic_eval(H_LINEAR, triple) == 8,
          "linear = 4 + 4 (deux tuiles a sortir), vu %d",
          heuristic_eval(H_LINEAR, triple));
  }
#endif

  TAP_END("heuristique");
}
