#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "enumerate.h"

#if WH_BOARD == 3

#define NPERM 362880L    /* 9! */

static const long FACT[10] = { 1,1,2,6,24,120,720,5040,40320,362880 };

long perm_rank(const cell_t *b)
{
  long rank = 0;
  int  i, j;

  for (i = 0; i < MAX_BOARD; i++) {
    int smaller = 0;
    for (j = i + 1; j < MAX_BOARD; j++)
      if (b[j] < b[i]) smaller++;
    rank += (long)smaller * FACT[MAX_BOARD - 1 - i];
  }
  return rank;
}

void perm_unrank(long rank, cell_t *b)
{
  cell_t pool[MAX_BOARD];
  int    i, k, n = MAX_BOARD;

  for (i = 0; i < MAX_BOARD; i++) pool[i] = (cell_t)i;

  for (i = 0; i < MAX_BOARD; i++) {
    long f = FACT[MAX_BOARD - 1 - i];
    int  idx = (int)(rank / f);
    rank %= f;
    b[i] = pool[idx];
    for (k = idx; k < n - 1; k++) pool[k] = pool[k + 1];
    n--;
  }
}

unsigned char *enum_depths(long *n_solvable, int *max_depth)
{
  const long cap = NPERM / 2 + 1;   /* 9!/2 est un théorème, +1 par prudence */
  unsigned char *depth = malloc(NPERM);
  cell_t        *queue = malloc((size_t)cap * MAX_BOARD);
  long           head = 0, tail = 0, count = 0;
  int            maxd = 0;
  cell_t         goal[MAX_BOARD];

  if (!depth || !queue) { free(depth); free(queue); return NULL; }
  memset(depth, ENUM_UNREACHABLE, NPERM);

  puzzle_goal(goal);
  depth[perm_rank(goal)] = 0;
  memcpy(queue, goal, MAX_BOARD);
  tail = 1;
  count = 1;

  while (head < tail) {
    cell_t cur[MAX_BOARD], next[MAX_BOARD];
    int    blank, move, d;

    memcpy(cur, queue + head * MAX_BOARD, MAX_BOARD);
    head++;
    d = depth[perm_rank(cur)];
    blank = puzzle_blank(cur);

    for (move = 0; move < MAX_MOVES; move++) {
      long r;

      if (puzzle_apply(cur, blank, move, next) < 0) continue;
      r = perm_rank(next);
      if (depth[r] != ENUM_UNREACHABLE) continue;

      /* La file est dimensionnée par un théorème. Si perm_rank cessait
         d'être injectif, le premier symptôme serait un débordement de tas
         silencieux, pas un test rouge. On préfère échouer proprement. */
      if (tail >= cap) { free(depth); free(queue); return NULL; }

      depth[r] = (unsigned char)(d + 1);
      if (d + 1 > maxd) maxd = d + 1;
      memcpy(queue + tail * MAX_BOARD, next, MAX_BOARD);
      tail++;
      count++;
    }
  }

  free(queue);
  if (n_solvable) *n_solvable = count;
  if (max_depth)  *max_depth  = maxd;
  return depth;
}

void enum_report(void)
{
  unsigned char *d;
  long  n = 0, hist[256] = { 0 }, i;
  int   maxd = 0, k;

  d = enum_depths(&n, &maxd);
  if (!d) { printf("enumeration impossible (memoire ou rang non injectif)\n"); return; }

  for (i = 0; i < NPERM; i++)
    if (d[i] != ENUM_UNREACHABLE) hist[d[i]]++;

  printf("Enumeration exhaustive du 8-puzzle\n");
  printf("  etats solvables     : %ld   (9!/2 = 181440)\n", n);
  printf("  profondeur maximale : %d coups\n\n", maxd);
  printf("  profondeur     etats     part\n");
  for (k = 0; k <= maxd; k++)
    printf("  %10d  %8ld  %6.3f %%\n", k, hist[k],
           100.0 * (double)hist[k] / (double)n);

  free(d);
}

/* Degré d'un sommet = nombre de coups légaux. Il ne dépend que de la case
   vide : 2 aux coins, 3 sur les bords, 4 au centre. */
static int degree_of_blank(int blank)
{
  cell_t dummy[MAX_BOARD], out[MAX_BOARD];
  int    move, n = 0;

  puzzle_goal(dummy);
  for (move = 0; move < MAX_MOVES; move++)
    if (puzzle_apply(dummy, blank, move, out) >= 0) n++;
  return n;
}

/* Parité de (ligne + colonne) de la case `p`. Chaque coup la change. */
static int cell_parity(int p)
{
  return ((p / WH_BOARD) + (p % WH_BOARD)) & 1;
}

/*
 * Khi-deux d'ajustement.
 *
 * Les classes d'effectif théorique inférieur à 5 sont réellement FUSIONNÉES
 * avec la suivante — on accumule observé et théorique et on ne clôt une
 * classe que lorsqu'elle atteint le seuil. Le résidu final est fusionné dans
 * la dernière classe close, et non érigé en classe supplémentaire : lui
 * donner son propre terme au dénominateur minuscule est exactement l'erreur
 * que la règle de regroupement existe pour éviter, et elle gonfle les ddl.
 *
 * Les classes d'effectif théorique nul dont l'observé n'est PAS nul sont
 * signalées : elles rendent le test invalide, et c'est précisément ce que
 * produit la parité si on l'oublie.
 */
static double chi2_fit(const long *obs, const double *expected, int n,
                       int *df, long *impossible_seen)
{
  double chi2 = 0.0, o = 0.0, e = 0.0, last_o = 0.0, last_e = 0.0;
  int    i, bins = 0;
  long   bad = 0;

  for (i = 0; i < n; i++) {
    if (expected[i] <= 0.0) {
      if (obs[i] > 0) bad += obs[i];   /* observé là où la loi dit impossible */
      continue;
    }
    o += (double)obs[i];
    e += expected[i];
    if (e >= 5.0) {
      chi2 += (o - e) * (o - e) / e;
      last_o = o; last_e = e;
      bins++;
      o = e = 0.0;
    }
  }

  if (e > 0.0 && bins > 0) {
    /* Fusion effective : on défait le dernier terme et on le recalcule. */
    chi2 -= (last_o - last_e) * (last_o - last_e) / last_e;
    o += last_o; e += last_e;
    chi2 += (o - e) * (o - e) / e;
  } else if (e > 0.0) {
    chi2 += (o - e) * (o - e) / e;
    bins++;
  }

  if (impossible_seen) *impossible_seen = bad;
  *df = bins > 1 ? bins - 1 : 1;
  return chi2;
}

/*
 * p-valeur par l'approximation de Wilson-Hilferty : (X/k)^(1/3) est
 * approximativement normale. Évite d'embarquer une table de quantiles.
 * Valable à partir de df >= 3.
 */
static double chi2_pvalue(double chi2, int df)
{
  double k = (double)df;
  double z = (pow(chi2 / k, 1.0 / 3.0) - (1.0 - 2.0 / (9.0 * k)))
             / sqrt(2.0 / (9.0 * k));
  return 0.5 * erfc(z / sqrt(2.0));
}

/* erfc sous-déborde à 0 vers x = 27 : au-delà, « p = 0.00e+00 » se lirait
   comme une p-valeur calculée alors qu'aucune ne l'a été. */
static void print_verdict(const char *label, double chi2, int df)
{
  double p = chi2_pvalue(chi2, df);

  printf("  %-42s khi-deux = %12.1f  ddl = %2d  ", label, chi2, df);
  if (p == 0.0) printf("p < 1e-300  -> REJETEE\n");
  else          printf("p = %8.2e  -> %s\n", p,
                       p < 0.01 ? "REJETEE" : "compatible");
}

void enum_generator_test(long samples, int shuffle, uint64_t seed)
{
  unsigned char *d;
  long   n = 0, i;
  int    maxd = 0, k, df;
  long   exact_hist[256] = { 0 }, obs_depth[256] = { 0 };
  long   obs_blank[MAX_BOARD] = { 0 };
  long   unreachable = 0, impossible = 0;
  double exp_depth[256];
  double exp_unif[MAX_BOARD], exp_deg[MAX_BOARD], exp_degpar[MAX_BOARD];
  double degsum = 0.0, degsum_par = 0.0, reachable_mass = 0.0;
  int    parity;
  rng_t  rng;
  cell_t b[MAX_BOARD];

  d = enum_depths(&n, &maxd);
  if (!d) { printf("enumeration impossible\n"); return; }

  for (i = 0; i < NPERM; i++)
    if (d[i] != ENUM_UNREACHABLE) exact_hist[d[i]]++;

  rng_seed(&rng, seed);
  for (i = 0; i < samples; i++) {
    unsigned char dd;

    puzzle_goal(b);
    puzzle_shuffle(b, shuffle, &rng);
    dd = d[perm_rank(b)];
    if (dd == ENUM_UNREACHABLE) { unreachable++; continue; }  /* jamais, mais
                                    obs_depth[255] ecraserait la pile */
    obs_depth[dd]++;
    obs_blank[puzzle_blank(b)]++;
  }

  printf("Generateur : %ld tirages de %d coups depuis le but (graine %llu)\n\n",
         samples, shuffle, (unsigned long long)seed);

  /* ---------------------------------------------------------------- */
  /* Le fait central : le graphe du taquin est biparti.                */
  /*                                                                    */
  /* Chaque coup echange la case vide avec une voisine, donc change la  */
  /* parite de (ligne + colonne) de la case vide. Apres un nombre PAIR  */
  /* de coups depuis le but, la case vide ne peut donc occuper qu'une   */
  /* case de meme parite que sa position de depart. La marche est       */
  /* periodique : elle n'a pas une loi limite, elle en a deux.          */
  /* ---------------------------------------------------------------- */
  parity = cell_parity(MAX_BOARD - 1) ^ (shuffle & 1);

  for (k = 0; k < MAX_BOARD; k++) {
    double deg = (double)degree_of_blank(k);
    degsum += deg;
    if (cell_parity(k) == parity) degsum_par += deg;
  }
  for (k = 0; k < MAX_BOARD; k++) {
    double deg = (double)degree_of_blank(k);
    exp_unif[k]   = (double)samples / (double)MAX_BOARD;
    exp_deg[k]    = (double)samples * deg / degsum;
    exp_degpar[k] = (cell_parity(k) == parity)
                    ? (double)samples * deg / degsum_par : 0.0;
  }

  printf("Position de la case vide\n");
  printf("  case  parite  degre     observe   H1 uniforme  H2 degre  H3 degre+parite\n");
  for (k = 0; k < MAX_BOARD; k++)
    printf("  %4d  %6d  %5d  %8.3f %%  %8.3f %% %8.3f %%   %8.3f %%\n",
           k, cell_parity(k), degree_of_blank(k),
           100.0 * (double)obs_blank[k] / (double)samples,
           100.0 * exp_unif[k]   / (double)samples,
           100.0 * exp_deg[k]    / (double)samples,
           100.0 * exp_degpar[k] / (double)samples);
  printf("\n");

  /*
   * L'ordre d'evaluation des arguments d'un appel n'est PAS specifie en C :
   * ecrire print_verdict(…, chi2_fit(…, &df), df) laisse le compilateur lire
   * `df` AVANT que chi2_fit ne l'ecrive. On sequence explicitement.
   */
  {
    double chi2 = chi2_fit(obs_blank, exp_unif, MAX_BOARD, &df, NULL);
    print_verdict("H1 : loi uniforme sur les etats solvables", chi2, df);
  }
  {
    double chi2 = chi2_fit(obs_blank, exp_deg, MAX_BOARD, &df, NULL);
    print_verdict("H2 : proportionnelle au degre", chi2, df);
  }
  {
    double chi2 = chi2_fit(obs_blank, exp_degpar, MAX_BOARD, &df, &impossible);
    print_verdict("H3 : proportionnelle au degre, a parite fixee", chi2, df);
    if (impossible)
      printf("  (%ld tirages hors de la classe de parite : H3 serait fausse)\n",
             impossible);
  }

  /* ---------------------------------------------------------------- */
  /* Meme correction pour la profondeur : apres N coups, seules les    */
  /* profondeurs optimales de meme parite que N sont atteignables.     */
  /* ---------------------------------------------------------------- */
  for (k = 0; k <= maxd; k++)
    if ((k & 1) == (shuffle & 1)) reachable_mass += (double)exact_hist[k];

  for (k = 0; k < 256; k++)
    exp_depth[k] = (k <= maxd && (k & 1) == (shuffle & 1))
                   ? (double)samples * (double)exact_hist[k] / reachable_mass
                   : 0.0;

  printf("\nProfondeur optimale, contre la distribution exacte\n");
  printf("  restreinte aux profondeurs de parite %d "
         "(%.1f %% de l'espace, le reste est hors d'atteinte en %d coups)\n",
         shuffle & 1, 100.0 * reachable_mass / (double)n, shuffle);
  {
    double chi2 = chi2_fit(obs_depth, exp_depth, 256, &df, &impossible);
    print_verdict("distribution exacte a parite fixee", chi2, df);
    if (impossible)
      printf("  (%ld tirages de parite impossible)\n", impossible);
  }
  if (unreachable)
    printf("  ATTENTION : %ld tirages hors du graphe atteignable\n", unreachable);

  free(d);
}

#else /* WH_BOARD != 3 */

long perm_rank(const cell_t *b) { (void)b; return -1; }
void perm_unrank(long r, cell_t *b) { (void)r; (void)b; }
unsigned char *enum_depths(long *n, int *m) { (void)n; (void)m; return NULL; }

void enum_report(void)
{
  fprintf(stderr,
    "erreur : --enumerate n'existe qu'en 3x3.\n"
    "  raison  : il enumere l'espace d'etats en entier. Le 8-puzzle a\n"
    "            181 440 etats solvables ; le %dx%d en a bien davantage.\n"
    "  a faire : ./bin/taquin --enumerate\n", WH_BOARD, WH_BOARD);
}

void enum_generator_test(long samples, int shuffle, uint64_t seed)
{
  fprintf(stderr,
    "erreur : --dist n'existe qu'en 3x3.\n"
    "  raison  : il compare les tirages a la distribution EXACTE des\n"
    "            profondeurs, obtenue par enumeration complete.\n"
    "  a faire : ./bin/taquin --dist %ld --shuffle %d --seed %llu\n",
    samples, shuffle, (unsigned long long)seed);
}

#endif
