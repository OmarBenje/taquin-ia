#ifndef STATS_H
#define STATS_H

#include <stddef.h>

/*
 * Ce qu'on mesure, et pourquoi.
 *
 *   generated   nœuds effectivement créés. C'est le coût mémoire.
 *   expanded    nœuds extraits de l'open list et développés. C'est le coût
 *               algorithmique : c'est ce nombre que l'heuristique fait
 *               baisser, et c'est lui qu'il faut comparer entre heuristiques
 *               (le temps, lui, dépend aussi des structures de données).
 *   duplicates  fils écartés parce que l'état était déjà connu.
 *   improved    fils qui ont amélioré le g d'un état déjà sur l'open list
 *               (voir --dup dans le README).
 *   open_max    taille maximale de l'open list : la borne mémoire d'A*, celle
 *               qui explose sur le 15-puzzle et qu'IDA* n'a pas.
 *   bytes_peak  mémoire pic réellement allouée par la recherche. Comptée par
 *               le solveur lui-même, pas par le processus, pour rester juste
 *               entre deux exécutions successives.
 */

typedef struct {
  long   generated;
  long   expanded;
  long   duplicates;
  long   improved;
  long   open_max;
  long   closed_size;
  size_t bytes_peak;
  double seconds;
  int    solution_len;   /* -1 si non résolu */
  int    iterations;     /* itérations d'approfondissement (IDA*) */
  int    aborted;        /* 1 si la limite --max-nodes a été atteinte */
} stats_t;

void   stats_reset(stats_t *s);

/* Horloge monotone, en secondes. */
double now_seconds(void);

/* High-water mark du processus, en octets (getrusage). */
size_t process_peak_rss(void);

#endif /* STATS_H */
