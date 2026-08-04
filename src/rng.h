#ifndef RNG_H
#define RNG_H

#include <stdint.h>

/*
 * Générateur pseudo-aléatoire déterministe et portable (splitmix64).
 *
 * Pourquoi ne pas garder rand() ?  Deux raisons de reproductibilité :
 *   1. rand() n'est pas spécifié : la suite produite par srand(42) diffère
 *      entre la libc de macOS et la glibc.  Un `--seed 42` ne rejouerait donc
 *      pas la même instance d'une machine à l'autre.
 *   2. La macro RANDMAX() du squelette,
 *        (int)((float)(x) * rand() / (RAND_MAX + 1.0))
 *      passe par un `float` : pour x = 4 et rand() proche de RAND_MAX, le
 *      produit 4 * 2147483647 = 8589934588 s'arrondit au float le plus proche,
 *      8589934592 = 4 * 2^31, et le résultat vaut exactement 4.0 — soit un
 *      indice hors bornes dans `neighbors[4]`.  Voir docs/RETROSPECTIVE.md et docs/DESIGN.md.
 */

typedef struct {
  uint64_t state;
} rng_t;

static inline void rng_seed(rng_t *r, uint64_t seed)
{
  r->state = seed + 0x9E3779B97F4A7C15ULL;
}

static inline uint64_t rng_next(rng_t *r)
{
  uint64_t z = (r->state += 0x9E3779B97F4A7C15ULL);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

/* Entier uniforme dans [0, n) — rejet pour éliminer le biais du modulo. */
static inline uint64_t rng_below(rng_t *r, uint64_t n)
{
  uint64_t limit, v;
  if (n <= 1) return 0;
  limit = UINT64_MAX - (UINT64_MAX % n) - 1;
  do { v = rng_next(r); } while (v > limit);
  return v % n;
}

#endif /* RNG_H */
