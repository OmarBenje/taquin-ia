#include <string.h>
#include "hashset.h"
#include "tap.h"

#define N 50000

static node_t  pool[N];
static cell_t  boards[N][MAX_BOARD];

int main(void)
{
  hset_t s;
  rng_t  rng;
  cell_t absent[MAX_BOARD];
  int    i, distinct = 0;

  hset_init(&s);
  rng_seed(&rng, 424242);

  /* On insere des plateaux atteignables, donc potentiellement repetes :
     on ne garde que les etats reellement nouveaux, comme le fait A*. */
  for (i = 0; i < N; i++) {
    puzzle_goal(boards[i]);
    puzzle_shuffle(boards[i], (int)rng_below(&rng, 200), &rng);

    if (hset_find(&s, boards[i])) continue;

    memcpy(pool[distinct].board, boards[i], MAX_BOARD);
    pool[distinct].hnext = NULL;
    hset_insert(&s, &pool[distinct]);
    distinct++;
  }
  CHECK(s.count == (size_t)distinct, "count doit suivre les insertions");

  /* Tout ce qui a ete insere doit se retrouver, a l'identique. */
  for (i = 0; i < distinct; i++) {
    node_t *f = hset_find(&s, pool[i].board);
    CHECK(f != NULL, "un plateau insere doit etre retrouve");
    CHECK(f && memcmp(f->board, pool[i].board, MAX_BOARD) == 0,
          "hset_find doit rendre le noeud du bon plateau");
  }

  /* L'etat but n'a pas ete insere (le melange fait au moins 0 coup, donc il
     peut l'etre) : on teste un plateau volontairement hors du graphe. */
  puzzle_goal(absent);
  absent[0] = (cell_t)(MAX_BOARD + 7);
  CHECK(hset_find(&s, absent) == NULL, "un plateau absent ne doit pas etre trouve");

  /* La table doit rester peu chargee : sinon la promesse O(1) est fausse. */
  CHECK(s.count <= s.nbuckets, "charge <= 1 : %zu elements pour %zu seaux",
        s.count, s.nbuckets);
  CHECK(hset_avg_probes(&s) < 3.0,
        "chaine moyenne trop longue : %.2f", hset_avg_probes(&s));

  printf("  (%d etats distincts, %zu seaux, %.2f noeud(s) examine(s) par recherche)\n",
         distinct, s.nbuckets, hset_avg_probes(&s));

  hset_free(&s);
  TAP_END("table de hachage");
}
