#ifndef TAP_H
#define TAP_H

#include <stdio.h>

/* Micro-harnais de test : pas de dépendance, un binaire par fichier de test. */

static int tap_ran = 0;
static int tap_failed = 0;

#define CHECK(cond, ...)                                            \
  do {                                                              \
    tap_ran++;                                                      \
    if (!(cond)) {                                                  \
      tap_failed++;                                                 \
      fprintf(stderr, "  ECHEC %s:%d : ", __FILE__, __LINE__);      \
      fprintf(stderr, __VA_ARGS__);                                 \
      fprintf(stderr, "\n");                                        \
    }                                                               \
  } while (0)

#define TAP_END(name)                                               \
  do {                                                              \
    printf("%-18s %4d verifications, %d echec(s)\n",                \
           (name), tap_ran, tap_failed);                            \
    return tap_failed ? 1 : 0;                                      \
  } while (0)

#endif /* TAP_H */
