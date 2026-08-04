#include <stdlib.h>
#include <assert.h>
#include "pqueue.h"

/* a passe avant b ? f croissant, puis g décroissant pour départager. */
static inline int before(const node_t *a, const node_t *b)
{
  int fa = NODE_F(a), fb = NODE_F(b);

  if (fa != fb) return fa < fb;
  return a->g > b->g;
}

static void place(heap_t *h, size_t i, node_t *x)
{
  h->v[i] = x;
  x->heap_idx = (int)i;
}

static void sift_up(heap_t *h, size_t i)
{
  node_t *x = h->v[i];

  while (i > 0) {
    size_t parent = (i - 1) / 2;
    if (!before(x, h->v[parent])) break;
    place(h, i, h->v[parent]);
    i = parent;
  }
  place(h, i, x);
}

static void sift_down(heap_t *h, size_t i)
{
  node_t *x = h->v[i];

  for (;;) {
    size_t  l = 2 * i + 1, r = l + 1, best = i;
    node_t *bestn = x;

    /* On compare à `x` et non à h->v[best] : après la première descente,
       h->v[i] contient encore l'ancienne valeur remontée, pas x. */
    if (l < h->n && before(h->v[l], bestn)) { best = l; bestn = h->v[l]; }
    if (r < h->n && before(h->v[r], bestn)) { best = r; bestn = h->v[r]; }
    if (best == i) break;

    place(h, i, bestn);
    i = best;
  }
  place(h, i, x);
}

void heap_init(heap_t *h)
{
  h->v = NULL;
  h->n = h->cap = h->bytes = 0;
}

void heap_free(heap_t *h)
{
  free(h->v);
  heap_init(h);
}

void heap_push(heap_t *h, node_t *x)
{
  if (h->n == h->cap) {
    size_t cap = h->cap ? h->cap * 2 : 1024;
    node_t **v = realloc(h->v, cap * sizeof(*v));
    assert(v);
    h->v = v;
    h->cap = cap;
    h->bytes = cap * sizeof(*v);
  }
  h->n++;
  place(h, h->n - 1, x);
  sift_up(h, h->n - 1);
}

node_t *heap_pop(heap_t *h)
{
  node_t *top;

  if (h->n == 0) return NULL;

  top = h->v[0];
  top->heap_idx = -1;
  h->n--;
  if (h->n > 0) {
    place(h, 0, h->v[h->n]);
    sift_down(h, 0);
  }
  return top;
}

void heap_decrease(heap_t *h, node_t *x)
{
  assert(x->heap_idx >= 0 && (size_t)x->heap_idx < h->n);
  sift_up(h, (size_t)x->heap_idx);
}
