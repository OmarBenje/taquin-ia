#ifndef HASHSET_H
#define HASHSET_H

#include <stddef.h>
#include "node.h"

/*
 * Table de hachage des états déjà rencontrés (open list et closed list
 * réunies : c'est exactement l'union que testait `onList(closed) ||
 * onList(open)`).
 *
 * Remplace onList(), qui parcourait la liste entière en faisant un memcmp par
 * nœud : O(n) par test, quatre tests par nœud développé, donc O(n²) sur la
 * recherche.
 *
 * Ce qu'on hache : les MAX_BOARD octets du plateau, et rien d'autre. Le
 * plateau détermine entièrement l'état (la position de la case vide s'en
 * déduit) — deux nœuds de même plateau sont le même état, quel que soit leur
 * g ou leur parent. La fonction est FNV-1a 64 bits.
 *
 * Collisions : chaînage par seau (node_t::hnext), avec un memcmp complet sur
 * chaque candidat — une égalité de hachage n'est jamais prise pour une
 * égalité d'état. Le nombre de seaux double dès que la charge dépasse 1,0, ce
 * qui garde les chaînes courtes ; hset_avg_probes() rend la longueur moyenne
 * réellement parcourue, mesurée et affichée par le banc.
 */
typedef struct {
  node_t      **buckets;
  size_t        nbuckets;
  size_t        count;
  size_t        bytes;
  unsigned long lookups;
  unsigned long probes;
} hset_t;

void    hset_init(hset_t *s);
void    hset_free(hset_t *s);
node_t *hset_find(hset_t *s, const cell_t *board);
void    hset_insert(hset_t *s, node_t *n);
double  hset_avg_probes(const hset_t *s);

#endif /* HASHSET_H */
