#include <stdlib.h>
#include "pqueue.h"
#include "tap.h"

#define N 20000

static node_t pool[N];

int main(void)
{
  heap_t h;
  rng_t  rng;
  int    i, prev_f, prev_g, popped;

  heap_init(&h);
  rng_seed(&rng, 987654321);

  for (i = 0; i < N; i++) {
    pool[i].g = (int)rng_below(&rng, 60);
    pool[i].h = (int)rng_below(&rng, 60);
    pool[i].heap_idx = -1;
    heap_push(&h, &pool[i]);
    CHECK(pool[i].heap_idx >= 0, "heap_idx doit etre renseigne a l'insertion");
  }
  CHECK(heap_size(&h) == N, "taille du tas apres %d insertions", N);

  /* Extraction : f croissant, et a f egal g decroissant. */
  prev_f = -1; prev_g = 1 << 30; popped = 0;
  for (;;) {
    node_t *x = heap_pop(&h);
    if (!x) break;
    popped++;
    CHECK(x->heap_idx == -1, "un noeud extrait n'est plus dans le tas");
    CHECK(NODE_F(x) >= prev_f, "f doit etre croissant : %d apres %d",
          NODE_F(x), prev_f);
    if (NODE_F(x) == prev_f)
      CHECK(x->g <= prev_g, "a f egal, g doit etre decroissant : %d apres %d",
            x->g, prev_g);
    prev_f = NODE_F(x); prev_g = x->g;
  }
  CHECK(popped == N, "on doit ressortir exactement %d noeuds, vu %d", N, popped);
  CHECK(heap_pop(&h) == NULL, "un tas vide rend NULL");

  /*
   * Verification de l'invariant du tas apres CHAQUE operation.
   *
   * Le simple controle « les f sortent croissants » ci-dessus ne suffit pas :
   * un sift_down fautif peut laisser un tas invalide qui rend quand meme les
   * elements dans le bon ordre la plupart du temps. C'est exactement le bug
   * qui s'etait glisse ici (comparaison a h->v[best] au lieu de x apres la
   * premiere descente), et que seule cette verification structurelle attrape
   * de facon fiable.
   */
  {
    int k, bad = 0;

    for (k = 0; k < 400; k++) {
      size_t j;
      pool[k].g = (int)rng_below(&rng, 40);
      pool[k].h = (int)rng_below(&rng, 40);
      pool[k].heap_idx = -1;
      heap_push(&h, &pool[k]);
      for (j = 1; j < heap_size(&h); j++)
        if (NODE_F(h.v[(j - 1) / 2]) > NODE_F(h.v[j])) bad++;
    }
    while (heap_size(&h) > 0) {
      size_t j;
      heap_pop(&h);
      for (j = 1; j < heap_size(&h); j++)
        if (NODE_F(h.v[(j - 1) / 2]) > NODE_F(h.v[j])) bad++;
    }
    CHECK(bad == 0, "invariant du tas viole %d fois", bad);
  }

  /* Chaque noeud doit connaitre sa propre position dans le tableau. */
  {
    size_t j;
    int    k, bad = 0;

    for (k = 0; k < 400; k++) {
      pool[k].g = (int)rng_below(&rng, 40);
      pool[k].h = (int)rng_below(&rng, 40);
      pool[k].heap_idx = -1;
      heap_push(&h, &pool[k]);
    }
    for (j = 0; j < heap_size(&h); j++)
      if (h.v[j]->heap_idx != (int)j) bad++;
    CHECK(bad == 0, "heap_idx desynchronise pour %d noeud(s)", bad);
    while (heap_pop(&h)) { /* vider */ }
  }

  /* decrease-key : baisser g doit faire remonter le noeud jusqu'a la racine. */
  for (i = 0; i < 500; i++) {
    pool[i].g = 100; pool[i].h = 100; pool[i].heap_idx = -1;
    heap_push(&h, &pool[i]);
  }
  pool[250].g = 0; pool[250].h = 0;
  heap_decrease(&h, &pool[250]);
  CHECK(heap_pop(&h) == &pool[250],
        "apres decrease-key, le noeud ameliore doit sortir en premier");

  heap_free(&h);
  TAP_END("tas binaire");
}
