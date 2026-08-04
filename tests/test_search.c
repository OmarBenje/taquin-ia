#include <string.h>
#include "search.h"
#include "tap.h"

/*
 * Le test qui compte : le BFS est l'oracle.
 *
 * Le BFS explore par profondeur croissante, la premiere solution qu'il trouve
 * est donc optimale par construction — il ne fait aucune hypothese sur
 * l'heuristique. Tout le reste (A*, chaque heuristique, chaque implementation)
 * doit rendre exactement la meme longueur.
 */

static int solve(algo_id algo, impl_id impl, heuristic_id h, dup_policy dup,
                 const cell_t *start, stats_t *st, int *path, int *len)
{
  search_opts o;

  memset(&o, 0, sizeof(o));
  o.algo = algo; o.impl = impl; o.heuristic = h; o.dup = dup; o.max_nodes = 0;
  return search_solve(&o, start, st, path, 512, len);
}

static int replay_ok(const cell_t *start, const int *path, int len)
{
  cell_t b[MAX_BOARD];
  int    blank, i;

  memcpy(b, start, MAX_BOARD);
  blank = puzzle_blank(b);
  for (i = 0; i < len; i++) {
    blank = puzzle_apply(b, blank, path[i], b);
    if (blank < 0) return 0;
  }
  return puzzle_is_goal(b);
}

int main(void)
{
  rng_t rng;
  int   run;
  long  expanded_by_h[H_COUNT] = { 0 };
  int   naive_runs = 0, naive_suboptimal = 0, naive_excess = 0;
  int   list_naive_runs = 0, list_naive_suboptimal = 0;

  rng_seed(&rng, 2026);

  for (run = 0; run < 120; run++) {
    cell_t  start[MAX_BOARD];
    stats_t ref, st, bl;
    int     path[512], len = 0, rlen = 0, blen = 0;
    heuristic_id h;

    /* Les implementations sur listes sont en O(n^2) : on ne les croise avec
       l'implementation rapide que sur les instances peu profondes, sinon la
       suite de tests devient impraticable — ce qui est deja un resultat. */
    const int cross_check_list = (run < 15);

    puzzle_goal(start);
    puzzle_shuffle(start, cross_check_list ? 6 + (int)rng_below(&rng, 8)
                                           : 12 + (int)rng_below(&rng, 10), &rng);

    CHECK(solve(ALGO_BFS, IMPL_FAST, H_ZERO, DUP_NAIVE,
                start, &ref, NULL, &rlen) == SEARCH_OK,
          "l'oracle BFS doit resoudre l'instance");

    /* Le BFS des deux implementations doit se comporter a l'identique :
       memes noeuds generes, memes noeuds developpes, pas seulement la meme
       longueur. C'est ce qui autorise a mesurer les noeuds avec l'une et le
       temps avec l'autre. */
    if (cross_check_list) {
      CHECK(solve(ALGO_BFS, IMPL_LIST, H_ZERO, DUP_NAIVE,
                  start, &bl, NULL, &blen) == SEARCH_OK, "BFS liste");
      CHECK(blen == rlen, "BFS liste = BFS rapide : %d vs %d", blen, rlen);
      CHECK(bl.generated == ref.generated && bl.expanded == ref.expanded,
            "BFS : %ld/%ld noeuds (liste) vs %ld/%ld (rapide)",
            bl.generated, bl.expanded, ref.generated, ref.expanded);
    }

    /* A* rend l'optimum quelle que soit l'heuristique admissible... */
    for (h = 0; h < H_COUNT; h++) {
      CHECK(solve(ALGO_ASTAR, IMPL_FAST, h, DUP_GCOMPARE,
                  start, &st, path, &len) == SEARCH_OK, "A* rapide");
      CHECK(len == rlen, "A*/%s rend %d coups, l'oracle en donne %d",
            heuristic_name(h), len, rlen);
      CHECK(replay_ok(start, path, len), "le chemin rendu doit mener au but");

      /* ...et h(depart) <= cout optimal : l'admissibilite, mesuree. */
      CHECK(heuristic_eval(h, start) <= rlen,
            "%s surestime : h=%d > optimum=%d",
            heuristic_name(h), heuristic_eval(h, start), rlen);

      expanded_by_h[h] += st.expanded;
    }

    /* Les deux implementations d'A* doivent rendre la meme longueur. */
    if (cross_check_list) {
      CHECK(solve(ALGO_ASTAR, IMPL_LIST, H_MANHATTAN, DUP_GCOMPARE,
                  start, &st, path, &len) == SEARCH_OK, "A* liste");
      CHECK(len == rlen, "A* liste rend %d coups, l'oracle en donne %d", len, rlen);
      CHECK(replay_ok(start, path, len), "le chemin rendu doit mener au but");
    }

    /*
     * Question 3 du README, mesuree plutot que supposee.
     *
     * Jeter un fils dont l'etat est deja connu SANS comparer les g, c'est ce
     * que faisait le code d'origine. Ce n'est pas correct en general : un
     * etat present sur l'open list n'a pas forcement son g optimal, et le
     * jeter fige un chemin plus long. On n'exige donc pas l'optimum ici — on
     * compte les cas ou la politique naive rate, et on verifie seulement
     * qu'elle ne rend jamais MIEUX que l'optimum (ce qui signalerait un
     * chemin invalide).
     */
    CHECK(solve(ALGO_ASTAR, IMPL_FAST, H_MANHATTAN, DUP_NAIVE,
                start, &st, path, &len) == SEARCH_OK, "A* dup=naive");
    CHECK(len >= rlen, "A*/naive rend %d coups, moins que l'optimum %d", len, rlen);
    CHECK(replay_ok(start, path, len), "le chemin naif doit quand meme mener au but");
    naive_runs++;
    if (len != rlen) { naive_suboptimal++; naive_excess += len - rlen; }

    /* Meme mesure sur les listes du squelette : c'est le comportement exact
       du code d'origine. */
    if (cross_check_list) {
      CHECK(solve(ALGO_ASTAR, IMPL_LIST, H_MANHATTAN, DUP_NAIVE,
                  start, &st, path, &len) == SEARCH_OK, "A* liste/naive");
      CHECK(len >= rlen, "A* liste/naive rend moins que l'optimum");
      list_naive_runs++;
      if (len != rlen) list_naive_suboptimal++;
    }
  }

  printf("  dup=naive (impl rapide)  : %d/%d instances non optimales"
         " (%+d coup(s) au total)\n",
         naive_suboptimal, naive_runs, naive_excess);
  printf("  dup=naive (impl liste)   : %d/%d instances non optimales\n",
         list_naive_suboptimal, list_naive_runs);
  printf("  noeuds developpes par heuristique (total) :");
  {
    heuristic_id h;
    for (h = 0; h < H_COUNT; h++)
      printf("  %s=%ld", heuristic_name(h), expanded_by_h[h]);
    printf("\n");
  }

  /*
   * Dominance, enoncee correctement.
   *
   * h1 <= h2 ne garantit PAS que A* muni de h2 developpe moins de noeuds que
   * A* muni de h1 sur chaque instance : les noeuds de cout f = C* peuvent
   * etre developpes ou non selon la facon dont on departage les egalites, et
   * une heuristique plus forte peut en toucher davantage. La garantie ne
   * porte que sur les noeuds de f < C*. Sur un echantillon, en revanche,
   * l'ordre est net.
   */
  {
    heuristic_id h;
    for (h = 1; h < H_COUNT; h++)
      CHECK(expanded_by_h[h] <= expanded_by_h[h - 1],
            "en cumule, %s developpe %ld noeuds, plus que %s (%ld)",
            heuristic_name(h), expanded_by_h[h],
            heuristic_name(h - 1), expanded_by_h[h - 1]);
  }

  /* La limite --max-nodes doit interrompre proprement. */
  {
    cell_t  start[MAX_BOARD];
    stats_t st;
    search_opts o;
    int     len = 0;

    memset(&o, 0, sizeof(o));
    o.algo = ALGO_BFS; o.impl = IMPL_FAST; o.heuristic = H_ZERO;
    o.dup = DUP_NAIVE; o.max_nodes = 100;

    puzzle_goal(start);
    puzzle_shuffle(start, 200, &rng);
    CHECK(search_solve(&o, start, &st, NULL, 0, &len) == SEARCH_ABORTED,
          "la limite de noeuds doit interrompre la recherche");
    CHECK(st.aborted == 1 && st.generated >= 100, "l'abandon doit etre signale");
  }

  TAP_END("recherche");
}
