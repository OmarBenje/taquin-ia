#ifndef NODE_H
#define NODE_H

#include <stddef.h>
#include "puzzle.h"

/*
 * Nœud de recherche de l'implémentation rapide.
 *
 * Trois différences avec la structure Item du squelette :
 *  - le plateau est inclus dans le nœud, pas alloué à part : une allocation
 *    au lieu de deux, et une seule ligne de cache à toucher ;
 *  - g et h sont des entiers, pas des flottants : les comparaisons de f sont
 *    exactes, et « f égaux » est une notion sûre pour départager ;
 *  - heap_idx permet le decrease-key en O(log n) quand on découvre un
 *    meilleur chemin vers un état déjà sur l'open list.
 */
typedef struct node_s {
  struct node_s *parent;
  struct node_s *hnext;      /* chaînage dans le seau de la table de hachage */
  int            g, h;
  int            heap_idx;   /* position dans le tas, -1 si hors open list */
  unsigned char  blank;
  unsigned char  closed;
  cell_t         board[MAX_BOARD];
} node_t;

#define NODE_F(n) ((n)->g + (n)->h)

/*
 * Allocateur par blocs. Une recherche ne libère jamais un nœud isolément :
 * même écarté comme doublon, un nœud reste référencé par la table de hachage.
 * On alloue donc par paquets et on rend tout d'un coup — ce qui supprime au
 * passage des centaines de milliers d'appels à malloc/free.
 */
typedef struct arena_block_s arena_block_t;

typedef struct {
  arena_block_t *head;
  size_t         used;        /* nœuds utilisés dans le bloc courant */
  size_t         per_block;
  size_t         count;       /* nœuds alloués au total */
  size_t         bytes;       /* octets réservés au total */
} arena_t;

void    arena_init(arena_t *a);
node_t *arena_new(arena_t *a);
void    arena_free(arena_t *a);

#endif /* NODE_H */
