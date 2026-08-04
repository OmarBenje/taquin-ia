#ifndef HEURISTIC_H
#define HEURISTIC_H

#include "puzzle.h"

/*
 * Les heuristiques comparées dans le banc.  Toutes rendent 0 sur l'état but
 * et sont admissibles (elles ne surestiment jamais le nombre de coups
 * restants) — voir README, « Les questions ».
 *
 *   H_ZERO       h = 0. A* dégénère en recherche à coût uniforme (≈ BFS).
 *                Sert de témoin : c'est la borne « aucune information ».
 *   H_MISPLACED  nombre de tuiles mal placées.
 *   H_MANHATTAN  somme des distances de Manhattan.
 *   H_LINEAR     Manhattan + 2 par conflit linéaire.
 *
 * Elles sont ordonnées par informativité croissante :
 *   H_ZERO <= H_MISPLACED <= H_MANHATTAN <= H_LINEAR  (pour tout plateau),
 * ce que tests/test_heuristic.c vérifie explicitement.
 */

typedef enum {
  H_ZERO = 0,
  H_MISPLACED,
  H_MANHATTAN,
  H_LINEAR,
  H_COUNT
} heuristic_id;

int         heuristic_eval(heuristic_id id, const cell_t *b);
const char *heuristic_name(heuristic_id id);

/* Rend 0 et remplit *out si le nom est reconnu, -1 sinon. */
int         heuristic_parse(const char *name, heuristic_id *out);

#endif /* HEURISTIC_H */
