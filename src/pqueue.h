#ifndef PQUEUE_H
#define PQUEUE_H

#include <stddef.h>
#include "node.h"

/*
 * Tas binaire min sur f = g + h.
 *
 * Remplace popBest() du squelette, qui parcourait toute l'open list à chaque
 * extraction — O(n) par coup, donc O(n²) sur la recherche entière.
 *
 * À f égal, on préfère le plus grand g. Sur un plateau de f identiques (très
 * fréquent avec Manhattan, qui varie de ±1 par coup), cela pousse la
 * recherche vers la profondeur plutôt que de l'étaler en largeur : on atteint
 * le but plus tôt sans perdre l'optimalité, puisque tous ces nœuds ont le
 * même f et qu'A* ne s'arrête qu'en extrayant le but.
 *
 * Chaque nœud connaît sa position dans le tas (heap_idx), ce qui rend le
 * decrease-key possible en O(log n) quand on découvre un meilleur chemin.
 */
typedef struct {
  node_t **v;
  size_t   n;
  size_t   cap;
  size_t   bytes;     /* octets réservés par le tableau */
} heap_t;

void    heap_init(heap_t *h);
void    heap_free(heap_t *h);
void    heap_push(heap_t *h, node_t *x);
node_t *heap_pop(heap_t *h);

/* À appeler après avoir diminué g (donc f) d'un nœud déjà dans le tas. */
void    heap_decrease(heap_t *h, node_t *x);

static inline size_t heap_size(const heap_t *h) { return h->n; }

#endif /* PQUEUE_H */
