#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "search.h"
#include "enumerate.h"

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
  int                enumerate;
  long long          dist_samples;
} cli_t;

static void usage(const char *prog)
{
  printf(
    "Usage : %s [options]\n"
    "\n"
    "  --algo bfs|astar|ida      algorithme de recherche      (defaut: astar)\n"
    "                            ida = approfondissement iteratif, memoire O(d)\n"
    "                            (il ignore --impl : ni tas, ni table d'etats)\n"
    "  --heuristic NOM           zero|misplaced|manhattan|linear (defaut: manhattan)\n"
    "  --impl list|fast          structures de donnees        (defaut: fast)\n"
    "                            list = listes du squelette (O(n) par test)\n"
    "                            fast = tas binaire + table de hachage\n"
    "  --dup naive|gcompare      doublons sur l'open list     (defaut: gcompare)\n"
    "  --seed N                  graine du generateur         (defaut: 42)\n"
    "  --runs N                  nombre d'instances           (defaut: 1)\n"
    "  --shuffle N               coups de melange par instance (defaut: 200)\n"
    "  --max-nodes N             abandonne au-dela de N noeuds generes (0 = illimite)\n"
    "  --max-bytes N             abandonne au-dela de N octets alloues   (0 = illimite)\n"
    "                            A* meurt en MEMOIRE, IDA* en TEMPS : bornez\n"
    "                            le premier en octets, le second en noeuds.\n"
    "  --csv FICHIER             ecrit une ligne par instance\n"
    "  --verify                  compare la longueur trouvee a l'oracle BFS\n"
    "  --show                    affiche la solution pas a pas\n"
    "  --quiet                   pas de sortie par instance\n"
    "\n"
    "Analyse (3x3 uniquement, sortent immediatement, aucun CSV) :\n"
    "  --enumerate               distribution exacte des profondeurs optimales\n"
    "  --dist N                  teste la loi du generateur sur N tirages\n"
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
  fprintf(f, "run,seed,size,algo,impl,heuristic,dup,shuffle,max_nodes,"
             "max_bytes,status,solution_len,optimal,generated,expanded,"
             "duplicates,improved,open_max,closed,bytes_peak,hash_probes,"
             "iterations,last_iter_generated,max_depth,seconds\n");
}

static void csv_row(FILE *f, const cli_t *c, int run, unsigned long long seed,
                    int rc, const stats_t *s, int optimal)
{
  fprintf(f, "%d,%llu,%d,%s,%s,%s,%s,%d,%ld,%zu,%s,"
             "%d,%d,%ld,%ld,%ld,%ld,%ld,%ld,%zu,%.3f,%d,%ld,%d,%.9f\n",
          run, seed, WH_BOARD,
          algo_name(c->o.algo), impl_name(c->o.impl),
          heuristic_name(c->o.heuristic), dup_name(c->o.dup),
          c->shuffle, c->o.max_nodes, c->o.max_bytes, status_name(rc),
          s->solution_len, optimal,
          s->generated, s->expanded, s->duplicates, s->improved,
          s->open_max, s->closed_size, s->bytes_peak, s->hash_probes,
          s->iterations, s->last_iter_generated, s->max_depth,
          s->seconds);
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
  int   i, run, impl_given = 0;
  long  tot_generated = 0, tot_expanded = 0, tot_open_max = 0, tot_improved = 0;
  double tot_seconds = 0.0, tot_probes = 0.0;
  long  tot_len = 0, solved = 0, aborted = 0, mismatches = 0, invalid = 0;
  size_t max_bytes = 0;

  memset(&c, 0, sizeof(c));
  c.o.algo      = ALGO_ASTAR;
  c.o.impl      = IMPL_FAST;
  c.o.heuristic = H_MANHATTAN;
  c.o.dup       = DUP_GCOMPARE;
  c.o.max_nodes = 0;
  c.seed        = 42;
  c.runs        = 1;
  c.shuffle     = 200;

#define WANT_VALUE()                                                        \
  do {                                                                      \
    if (i + 1 >= argc) {                                                    \
      fprintf(stderr, "%s attend une valeur\n", a); return 2;               \
    }                                                                       \
    v = argv[++i];                                                          \
  } while (0)

/* Énumère les valeurs légales : elles existent déjà dans les fonctions
   *_name(), ne pas les afficher est un choix, pas une contrainte. */
#define BAD_VALUE(lister, count) do {                                       \
    int k_;                                                                 \
    fprintf(stderr, "valeur invalide pour %s : \"%s\" (attendu :", a, v);   \
    for (k_ = 0; k_ < (count); k_++)                                        \
      fprintf(stderr, "%s %s", k_ ? "," : "", lister(k_));                  \
    fprintf(stderr, ")\n");                                                 \
    return 2;                                                               \
  } while (0)

/*
 * atoi() rend 0 sur une saisie non numérique, sans un mot. `--runs abc`
 * produisait donc un résultat, et `--seed lol` une graine 0 silencieuse —
 * indiscernable d'un `--seed 0` voulu. Sur un outil dont l'argument est
 * « chaque chiffre est traçable », c'était une contradiction interne.
 */
#define WANT_LONG(dest, lo, hi)                                             \
  do {                                                                      \
    char *end_; long long n_;                                               \
    WANT_VALUE();                                                           \
    n_ = strtoll(v, &end_, 10);                                             \
    if (*v == '\0' || *end_ != '\0' || n_ < (lo) || n_ > (hi)) {            \
      fprintf(stderr, "valeur invalide pour %s : \"%s\" "                   \
                      "(entier attendu entre %lld et %lld)\n",              \
              a, v, (long long)(lo), (long long)(hi));                      \
      return 2;                                                             \
    }                                                                       \
    (dest) = n_;                                                            \
  } while (0)

  for (i = 1; i < argc; i++) {
    const char *a = argv[i];
    const char *v = NULL;

    if (!strcmp(a, "--algo")) {
      WANT_VALUE();
      if (algo_parse(v, &c.o.algo)) BAD_VALUE(algo_name, ALGO_COUNT);
    } else if (!strcmp(a, "--heuristic")) {
      WANT_VALUE();
      if (heuristic_parse(v, &c.o.heuristic)) BAD_VALUE(heuristic_name, H_COUNT);
    } else if (!strcmp(a, "--impl")) {
      WANT_VALUE();
      if (impl_parse(v, &c.o.impl)) BAD_VALUE(impl_name, IMPL_COUNT);
      impl_given = 1;
    } else if (!strcmp(a, "--dup")) {
      WANT_VALUE();
      if (dup_parse(v, &c.o.dup)) BAD_VALUE(dup_name, DUP_COUNT);
    } else if (!strcmp(a, "--seed")) {
      char *end;
      WANT_VALUE();
      c.seed = strtoull(v, &end, 10);
      if (*v == '\0' || *end != '\0') {
        fprintf(stderr, "valeur invalide pour --seed : \"%s\" "
                        "(entier non signe attendu)\n", v);
        return 2;
      }
    } else if (!strcmp(a, "--runs")) {
      WANT_LONG(c.runs, 1, 10000000);
    } else if (!strcmp(a, "--shuffle")) {
      WANT_LONG(c.shuffle, 0, 1000000);
    } else if (!strcmp(a, "--max-nodes")) {
      WANT_LONG(c.o.max_nodes, 0, 1000000000000LL);
    } else if (!strcmp(a, "--max-bytes")) {
      long long mb;
      WANT_LONG(mb, 0, 1000000000000LL);
      c.o.max_bytes = (size_t)mb;
    } else if (!strcmp(a, "--csv")) {
      WANT_VALUE(); c.csv = v;
    } else if (!strcmp(a, "--verify")) {
      c.verify = 1;
    } else if (!strcmp(a, "--show")) {
      c.show = 1;
    } else if (!strcmp(a, "--quiet")) {
      c.quiet = 1;
    } else if (!strcmp(a, "--enumerate")) {
      c.enumerate = 1;
    } else if (!strcmp(a, "--dist")) {
      WANT_LONG(c.dist_samples, 1, 100000000);
    } else if (!strcmp(a, "--help") || !strcmp(a, "-h")) {
      usage(argv[0]); return 0;
    } else {
      fprintf(stderr, "option inconnue : %s\n", a);
      usage(argv[0]);
      return 2;
    }
  }

  /*
   * IDA* n'a ni file de priorité ni table d'états : --impl ne le concerne
   * pas. Le laisser passer en silence produirait des lignes de CSV portant
   * impl=list alors qu'aucune liste n'a été touchée — et le CSV est censé
   * être la matière première du README.
   */
  if (c.o.algo == ALGO_IDA) {
    if (impl_given)
      fprintf(stderr, "note : --impl est ignore avec --algo ida (ni tas, ni "
                      "table d'etats). La colonne CSV vaudra \"n/a\".\n");
    c.o.impl = IMPL_NA;
  }

  /* Modes d'analyse : ils sortent immediatement et n'ecrivent aucun CSV. */
  if (c.enumerate) { enum_report(); return WH_BOARD == 3 ? 0 : 2; }
  if (c.dist_samples > 0) {
    enum_generator_test((long)c.dist_samples, c.shuffle, c.seed);
    return WH_BOARD == 3 ? 0 : 2;
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
    tot_improved  += st.improved;
    tot_seconds   += st.seconds;
    tot_probes    += st.hash_probes;
    if (st.open_max > tot_open_max) tot_open_max = st.open_max;
    if (st.bytes_peak > max_bytes)  max_bytes = st.bytes_peak;

    /* Oracle : BFS explore par profondeur croissante, la premiere solution
       qu'il trouve est donc optimale par construction. C'est lui qui prouve
       que l'A* et son heuristique rendent bien l'optimum. */
    if (c.verify) {
      search_opts oo = c.o;
      stats_t ost;

      oo.algo      = ALGO_BFS;
      oo.heuristic = H_ZERO;
      oo.impl      = IMPL_FAST;   /* jamais IMPL_NA, herite d'un --algo ida */
      oo.max_bytes = 0;
      oo.max_nodes = 0;
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
  printf("  chemins ameliores   : %ld  (fils trouvant un meilleur g)\n", tot_improved);
  if (c.o.impl == IMPL_FAST)
    printf("  chaine de hachage   : %.3f noeud(s) examine(s) par recherche\n",
           tot_probes / c.runs);
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
