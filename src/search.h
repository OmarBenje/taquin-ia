#ifndef SEARCH_H
#define SEARCH_H

#include "puzzle.h"
#include "heuristic.h"
#include "stats.h"

/*
 * ALGO_IDA : approfondissement itératif sur f = g + h.
 *
 * Il n'a PAS de table d'états visités — c'est elle qui ramènerait la mémoire
 * à O(b^d) et ferait perdre tout l'intérêt. La seule élimination est
 * l'anti-retour immédiat, donc un même état peut être revisité et chaque
 * itération rejoue ce que la précédente avait déjà exploré.
 *
 * Conséquence à connaître : IDA* est incomplet « vers le négatif ». Sur un
 * plateau insoluble il relève son seuil indéfiniment sans jamais conclure.
 * solve_ida teste donc la solvabilité par la parité des inversions avant de
 * chercher, plutôt que de laisser la recherche tourner sans fin.
 */
typedef enum { ALGO_BFS = 0, ALGO_ASTAR, ALGO_IDA, ALGO_COUNT } algo_id;

/*
 * IMPL_LIST : les listes chaînées du squelette (starting-kit/list.c).
 *             popBest est en O(n), onList en O(n) avec un memcmp par nœud —
 *             A* est donc en O(n²) sur la taille des listes. C'est le « avant ».
 * IMPL_FAST : tas binaire (O(log n) par extraction), table de hachage (O(1)
 *             par test d'appartenance) et allocateur par blocs. C'est le
 *             « après ». Les deux doivent rendre la même longueur de solution.
 */
typedef enum { IMPL_LIST = 0, IMPL_FAST, IMPL_COUNT, IMPL_NA } impl_id;

/*
 * Que faire d'un fils dont l'état est déjà connu ?
 *   DUP_NAIVE    on le jette sans regarder son g — c'est ce que faisait le
 *                code d'origine.
 *   DUP_GCOMPARE on compare les g et on remonte le meilleur chemin.
 *                Voir README, question 3.
 */
typedef enum { DUP_NAIVE = 0, DUP_GCOMPARE, DUP_COUNT } dup_policy;

typedef struct {
  algo_id      algo;
  impl_id      impl;
  heuristic_id heuristic;
  dup_policy   dup;

  /*
   * Deux bornes, parce que les deux algorithmes meurent de causes
   * différentes : A* meurt en MÉMOIRE, IDA* meurt en TEMPS. Un compteur
   * unique ne peut pas arbitrer les deux — le borner en nœuds générés
   * pénalise IDA*, qui les cumule sur toutes ses itérations, et laisse
   * passer un A* qui a déjà avalé des centaines de mégaoctets.
   */
  long         max_nodes;    /* 0 = illimité */
  size_t       max_bytes;    /* 0 = illimité */
} search_opts;

#define SEARCH_OK       0
#define SEARCH_NOSOL   (-1)
#define SEARCH_ABORTED (-2)

/*
 * Résout `start`. `path` (optionnel) reçoit la suite de coups MOVE_*.
 * Rend SEARCH_OK, SEARCH_NOSOL ou SEARCH_ABORTED.
 */
int search_solve(const search_opts *o, const cell_t *start,
                 stats_t *st, int *path, int path_cap, int *path_len);

const char *algo_name(algo_id id);
int         algo_parse(const char *name, algo_id *out);
const char *impl_name(impl_id id);
int         impl_parse(const char *name, impl_id *out);
const char *dup_name(dup_policy d);
int         dup_parse(const char *name, dup_policy *out);

#endif /* SEARCH_H */
