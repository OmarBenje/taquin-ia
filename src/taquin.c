#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "search.h"

/*
 * Banc de mesure du solveur.
 *
 *   ./taquin --algo astar --heuristic manhattan --seed 42 --runs 1000 --csv out.csv
 *
 * Une ligne de CSV par instance résolue : c'est ce fichier qui alimente tous
 * les tableaux du README. Rien n'est mesuré « à la main ».
 */

#define PATH_CAP 512

typedef struct {
  search_opts        o;
  unsigned long long seed;
  int                runs;
  int                shuffle;
  const char        *csv;
  int                verify;
  int                show;
  int                quiet;
} cli_t;

static void usage(const char *prog)
{
  printf(
    "Usage : %s [options]\n"
    "\n"
    "  --algo bfs|astar          algorithme de recherche      (defaut: astar)\n"
    "  --heuristic NOM           zero|misplaced|manhattan|linear (defaut: manhattan)\n"
    "  --impl list               structures de donnees        (defaut: list)\n"
    "  --dup naive|gcompare      doublons sur l'open list     (defaut: gcompare)\n"
    "  --seed N                  graine du generateur         (defaut: 42)\n"
    "  --runs N                  nombre d'instances           (defaut: 1)\n"
    "  --shuffle N               coups de melange par instance (defaut: 200)\n"
    "  --max-nodes N             abandonne au-dela de N noeuds generes (0 = illimite)\n"
    "  --csv FICHIER             ecrit une ligne par instance\n"
    "  --verify                  compare la longueur trouvee a l'oracle BFS\n"
    "  --show                    affiche la solution pas a pas\n"
    "  --quiet                   pas de sortie par instance\n"
    "  --help\n"
    "\n"
    "Plateau compile : %dx%d\n",
    prog, WH_BOARD, WH_BOARD);
}

/* ------------------------------------------------------------------ */

static const char *status_name(int rc)
{
  switch (rc) {
    case SEARCH_OK:      return "ok";
    case SEARCH_NOSOL:   return "nosol";
    case SEARCH_ABORTED: return "aborted";
    default:             return "?";
  }
}

static void csv_header(FILE *f)
{
  fprintf(f, "run,seed,size,algo,impl,heuristic,dup,shuffle,status,"
             "solution_len,optimal,generated,expanded,duplicates,improved,"
             "open_max,closed,bytes_peak,seconds\n");
}

static void csv_row(FILE *f, const cli_t *c, int run, unsigned long long seed,
                    int rc, const stats_t *s, int optimal)
{
  fprintf(f, "%d,%llu,%d,%s,%s,%s,%s,%d,%s,%d,%d,%ld,%ld,%ld,%ld,%ld,%ld,%zu,%.6f\n",
          run, seed, WH_BOARD,
          algo_name(c->o.algo), impl_name(c->o.impl),
          heuristic_name(c->o.heuristic), dup_name(c->o.dup),
          c->shuffle, status_name(rc),
          s->solution_len, optimal,
          s->generated, s->expanded, s->duplicates, s->improved,
          s->open_max, s->closed_size, s->bytes_peak, s->seconds);
}

static void show_solution(const cell_t *start, const int *path, int len)
{
  cell_t b[MAX_BOARD];
  int    blank, i;

  memcpy(b, start, MAX_BOARD);
  blank = puzzle_blank(b);

  printf("\nSolution en %d coups :\n", len);
  puzzle_print(b);
  for (i = 0; i < len; i++) {
    blank = puzzle_apply(b, blank, path[i], b);
    printf("coup %d : %s", i + 1, puzzle_move_name(path[i]));
    puzzle_print(b);
  }
}

/* Rejoue le chemin pour verifier qu'il mene reellement au but. */
static int path_is_valid(const cell_t *start, const int *path, int len)
{
  cell_t b[MAX_BOARD];
  int    blank, i;

  memcpy(b, start, MAX_BOARD);
  blank = puzzle_blank(b);
  for (i = 0; i < len; i++) {
    blank = puzzle_apply(b, blank, path[i], b);
    if (blank < 0) return 0;
  }
  return puzzle_is_goal(b);
}

/* ------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
  cli_t c;
  FILE *csv = NULL;
  int   i, run;
  long  tot_generated = 0, tot_expanded = 0, tot_open_max = 0;
  double tot_seconds = 0.0;
  long  tot_len = 0, solved = 0, aborted = 0, mismatches = 0, invalid = 0;
  size_t max_bytes = 0;

  memset(&c, 0, sizeof(c));
  c.o.algo      = ALGO_ASTAR;
  c.o.impl      = IMPL_LIST;
  c.o.heuristic = H_MANHATTAN;
  c.o.dup       = DUP_GCOMPARE;
  c.o.max_nodes = 0;
  c.seed        = 42;
  c.runs        = 1;
  c.shuffle     = 200;

#define WANT_VALUE()                                                        \
  do {                                                                      \
    if (i + 1 >= argc) {                                                    \
      fprintf(stderr, "%s attend une valeur\n", a); return 1;               \
    }                                                                       \
    v = argv[++i];                                                          \
  } while (0)

#define BAD_VALUE() do {                                                    \
    fprintf(stderr, "valeur invalide pour %s : %s\n", a, v); return 1;      \
  } while (0)

  for (i = 1; i < argc; i++) {
    const char *a = argv[i];
    const char *v = NULL;

    if (!strcmp(a, "--algo")) {
      WANT_VALUE();
      if (algo_parse(v, &c.o.algo)) BAD_VALUE();
    } else if (!strcmp(a, "--heuristic")) {
      WANT_VALUE();
      if (heuristic_parse(v, &c.o.heuristic)) BAD_VALUE();
    } else if (!strcmp(a, "--impl")) {
      WANT_VALUE();
      if (impl_parse(v, &c.o.impl)) BAD_VALUE();
    } else if (!strcmp(a, "--dup")) {
      WANT_VALUE();
      if (dup_parse(v, &c.o.dup)) BAD_VALUE();
    } else if (!strcmp(a, "--seed")) {
      WANT_VALUE(); c.seed = strtoull(v, NULL, 10);
    } else if (!strcmp(a, "--runs")) {
      WANT_VALUE(); c.runs = atoi(v);
    } else if (!strcmp(a, "--shuffle")) {
      WANT_VALUE(); c.shuffle = atoi(v);
    } else if (!strcmp(a, "--max-nodes")) {
      WANT_VALUE(); c.o.max_nodes = atol(v);
    } else if (!strcmp(a, "--csv")) {
      WANT_VALUE(); c.csv = v;
    } else if (!strcmp(a, "--verify")) {
      c.verify = 1;
    } else if (!strcmp(a, "--show")) {
      c.show = 1;
    } else if (!strcmp(a, "--quiet")) {
      c.quiet = 1;
    } else if (!strcmp(a, "--help")) {
      usage(argv[0]); return 0;
    } else {
      fprintf(stderr, "option inconnue : %s\n", a);
      usage(argv[0]);
      return 1;
    }
  }

  if (c.runs < 1) c.runs = 1;

  if (c.csv) {
    csv = fopen(c.csv, "w");
    if (!csv) { perror(c.csv); return 1; }
    csv_header(csv);
  }

  if (!c.quiet)
    printf("taquin %dx%d | algo=%s impl=%s heuristique=%s dup=%s "
           "melange=%d graines=%llu..%llu\n",
           WH_BOARD, WH_BOARD, algo_name(c.o.algo), impl_name(c.o.impl),
           heuristic_name(c.o.heuristic), dup_name(c.o.dup), c.shuffle,
           c.seed, c.seed + (unsigned long long)c.runs - 1);

  for (run = 0; run < c.runs; run++) {
    unsigned long long seed = c.seed + (unsigned long long)run;
    cell_t  start[MAX_BOARD];
    stats_t st;
    int     path[PATH_CAP], len = 0, rc, optimal = -1;
    rng_t   rng;

    rng_seed(&rng, seed);
    puzzle_goal(start);
    puzzle_shuffle(start, c.shuffle, &rng);

    rc = search_solve(&c.o, start, &st, path, PATH_CAP, &len);

    if (rc == SEARCH_OK) {
      solved++;
      tot_len += st.solution_len;
      if (!path_is_valid(start, path, len)) invalid++;
    }
    if (rc == SEARCH_ABORTED) aborted++;

    tot_generated += st.generated;
    tot_expanded  += st.expanded;
    tot_seconds   += st.seconds;
    if (st.open_max > tot_open_max) tot_open_max = st.open_max;
    if (st.bytes_peak > max_bytes)  max_bytes = st.bytes_peak;

    /* Oracle : BFS explore par profondeur croissante, la premiere solution
       qu'il trouve est donc optimale par construction. C'est lui qui prouve
       que l'A* et son heuristique rendent bien l'optimum. */
    if (c.verify) {
      search_opts oo = c.o;
      stats_t ost;

      oo.algo = ALGO_BFS;
      oo.heuristic = H_ZERO;
      if (search_solve(&oo, start, &ost, NULL, 0, NULL) == SEARCH_OK) {
        optimal = ost.solution_len;
        if (rc == SEARCH_OK && optimal != st.solution_len) mismatches++;
      }
    }

    if (csv) csv_row(csv, &c, run, seed, rc, &st, optimal);

    if (!c.quiet)
      printf("run %-4d seed=%-6llu %-7s len=%-3d generes=%-9ld developpes=%-9ld "
             "open_max=%-8ld memoire=%6.1f Ko  %8.3f ms\n",
             run, seed, status_name(rc), st.solution_len,
             st.generated, st.expanded, st.open_max,
             (double)st.bytes_peak / 1024.0, st.seconds * 1e3);

    if (c.show && rc == SEARCH_OK) show_solution(start, path, len);
  }

  if (csv) fclose(csv);

  printf("\n--- resume sur %d instance(s) ---\n", c.runs);
  printf("  resolues            : %ld", solved);
  if (aborted) printf("  (abandonnees : %ld)", aborted);
  printf("\n");
  if (solved)
    printf("  longueur moyenne    : %.2f coups\n", (double)tot_len / (double)solved);
  printf("  noeuds generes      : %ld  (moyenne %.0f)\n",
         tot_generated, (double)tot_generated / c.runs);
  printf("  noeuds developpes   : %ld  (moyenne %.0f)\n",
         tot_expanded, (double)tot_expanded / c.runs);
  printf("  open list max       : %ld noeuds\n", tot_open_max);
  printf("  memoire pic         : %.1f Ko (recherche) / %.1f Ko (processus)\n",
         (double)max_bytes / 1024.0, (double)process_peak_rss() / 1024.0);
  printf("  temps total         : %.3f s  (moyenne %.3f ms/instance)\n",
         tot_seconds, tot_seconds * 1e3 / c.runs);
  if (tot_seconds > 0.0)
    printf("  debit               : %.0f noeuds generes/s\n",
           (double)tot_generated / tot_seconds);
  if (invalid)
    printf("  CHEMINS INVALIDES   : %ld\n", invalid);
  if (c.verify)
    printf("  ecarts a l'oracle   : %ld  %s\n", mismatches,
           mismatches ? "<-- NON OPTIMAL" : "(toutes les solutions sont optimales)");

  return (mismatches || invalid) ? 1 : 0;
}
