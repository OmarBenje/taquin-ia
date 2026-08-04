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
  double hash_probes;    /* longueur de chaîne moyenne parcourue (impl fast) */
  double seconds;

  /*
   * Propres à IDA*.
   *
   * last_iter_generated : nœuds engendrés par la DERNIÈRE itération.
   *   generated / last_iter_generated est le facteur de ré-exploration, et
   *   c'est un chiffre, pas une intuition.
   * max_depth : profondeur maximale atteinte. Pour IDA*, open_max vaut
   *   max_depth + 1 : la pile est sa seule « open list », et c'est en
   *   nœuds retenus simultanément qu'on peut honnêtement le comparer à A*.
   *
   * Attention en lisant les CSV : pour A*, `expanded` compte des états
   * distincts ; pour IDA*, il compte des VISITES, ré-explorations comprises,
   * cumulées sur toutes les itérations. Les deux colonnes n'ont pas le même
   * sens selon l'algorithme.
   */
  long   last_iter_generated;
  int    max_depth;
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
