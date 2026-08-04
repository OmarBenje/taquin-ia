#include <stdlib.h>
#include <assert.h>
#include "node.h"

#define ARENA_BLOCK 8192

struct arena_block_s {
  struct arena_block_s *next;
  node_t                nodes[ARENA_BLOCK];
};

void arena_init(arena_t *a)
{
  a->head      = NULL;
  a->used      = ARENA_BLOCK;   /* force l'allocation du premier bloc */
  a->per_block = ARENA_BLOCK;
  a->count     = 0;
  a->bytes     = 0;
}

node_t *arena_new(arena_t *a)
{
  if (a->used == a->per_block) {
    arena_block_t *b = malloc(sizeof(*b));
    assert(b);
    b->next  = a->head;
    a->head  = b;
    a->used  = 0;
    a->bytes += sizeof(*b);
  }
  a->count++;
  return &a->head->nodes[a->used++];
}

void arena_free(arena_t *a)
{
  arena_block_t *b = a->head;

  while (b) {
    arena_block_t *next = b->next;
    free(b);
    b = next;
  }
  arena_init(a);
}
