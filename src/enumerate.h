#ifndef ENUMERATE_H
#define ENUMERATE_H

#include "puzzle.h"

/*
 * Le 8-puzzle n'a que 9! = 362 880 permutations, dont la moitié est
 * atteignable depuis l'état but. C'est assez petit pour être énuméré en
 * entier, ce qui donne la distribution EXACTE des profondeurs optimales — et
 * donc de quoi juger le générateur d'instances sur pièces plutôt que sur une
 * intuition.
 *
 * Réservé au 3x3 : le 15-puzzle a 16!/2 ≈ 1,05e13 états.
 */

/* Rang de Lehmer du plateau dans [0, 9![. Bijectif sur les permutations. */
long perm_rank(const cell_t *b);

/* Opération inverse : écrit dans `b` la permutation de rang `rank`. */
void perm_unrank(long rank, cell_t *b);

/*
 * BFS complet depuis l'état but. Rend un tableau de 9! octets : la profondeur
 * optimale de chaque état atteignable, ENUM_UNREACHABLE pour les autres.
 * À libérer par l'appelant, NULL si l'allocation échoue.
 */
#define ENUM_UNREACHABLE 0xFF
unsigned char *enum_depths(long *n_solvable, int *max_depth);

/* Imprime la distribution exacte des profondeurs optimales. */
void enum_report(void);

/*
 * Tire `samples` plateaux avec le générateur (`shuffle` coups depuis le but)
 * et confronte la loi obtenue à trois hypothèses. Voir enumerate.c pour
 * lesquelles, et pourquoi la troisième est la bonne.
 */
void enum_generator_test(long samples, int shuffle, uint64_t seed);

#endif /* ENUMERATE_H */
