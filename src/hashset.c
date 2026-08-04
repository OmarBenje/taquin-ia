#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "hashset.h"

#define HSET_INIT_BUCKETS 4096

/* FNV-1a 64 bits sur les MAX_BOARD octets du plateau. */
static uint64_t board_hash(const cell_t *b)
{
  uint64_t h = 1469598103934665603ULL;
  int i;

  for (i = 0; i < MAX_BOARD; i++) {
    h ^= (uint64_t)b[i];
    h *= 1099511628211ULL;
  }
  return h;
}

static void alloc_buckets(hset_t *s, size_t n)
{
  s->buckets  = calloc(n, sizeof(*s->buckets));
  assert(s->buckets);
  s->nbuckets = n;
  s->bytes    = n * sizeof(*s->buckets);
}

void hset_init(hset_t *s)
{
  s->buckets = NULL;
  s->nbuckets = s->count = s->bytes = 0;
  s->lookups = s->probes = 0;
  alloc_buckets(s, HSET_INIT_BUCKETS);
}

void hset_free(hset_t *s)
{
  free(s->buckets);
  s->buckets = NULL;
  s->nbuckets = s->count = s->bytes = 0;
}

/* Le nombre de seaux est une puissance de deux : le modulo est un masque. */
static size_t slot(const hset_t *s, uint64_t h)
{
  return (size_t)(h & (uint64_t)(s->nbuckets - 1));
}

static void grow(hset_t *s)
{
  node_t **old = s->buckets;
  size_t   oldn = s->nbuckets, i;

  alloc_buckets(s, oldn * 2);

  for (i = 0; i < oldn; i++) {
    node_t *n = old[i];
    while (n) {
      node_t *next = n->hnext;
      size_t  k = slot(s, board_hash(n->board));
      n->hnext = s->buckets[k];
      s->buckets[k] = n;
      n = next;
    }
  }
  free(old);
}

node_t *hset_find(hset_t *s, const cell_t *board)
{
  node_t *n = s->buckets[slot(s, board_hash(board))];

  s->lookups++;
  while (n) {
    s->probes++;
    /* Une égalité de hachage ne suffit pas : on compare le plateau entier. */
    if (memcmp(n->board, board, MAX_BOARD) == 0) return n;
    n = n->hnext;
  }
  return NULL;
}

void hset_insert(hset_t *s, node_t *n)
{
  size_t k;

  if (s->count + 1 > s->nbuckets) grow(s);   /* charge maximale : 1,0 */

  k = slot(s, board_hash(n->board));
  n->hnext = s->buckets[k];
  s->buckets[k] = n;
  s->count++;
}

double hset_avg_probes(const hset_t *s)
{
  return s->lookups ? (double)s->probes / (double)s->lookups : 0.0;
}
