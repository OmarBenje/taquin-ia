#include <string.h>
#include "heuristic.h"

/* md[tuile][case] = distance de Manhattan de `tuile` si elle est en `case`. */
static int  md[MAX_BOARD][MAX_BOARD];
static int  md_ready = 0;

static void md_init(void)
{
  int tile, pos;

  for (tile = 1; tile < MAX_BOARD; tile++) {
    int goal = tile - 1;                 /* la tuile t a pour but l'indice t-1 */
    for (pos = 0; pos < MAX_BOARD; pos++) {
      int dr = pos / WH_BOARD - goal / WH_BOARD;
      int dc = pos % WH_BOARD - goal % WH_BOARD;
      md[tile][pos] = (dr < 0 ? -dr : dr) + (dc < 0 ? -dc : dc);
    }
  }
  for (pos = 0; pos < MAX_BOARD; pos++)
    md[0][pos] = 0;                      /* la case vide ne compte pas */

  md_ready = 1;
}

static int misplaced(const cell_t *b)
{
  int i, n = 0;

  for (i = 0; i < MAX_BOARD; i++)
    if (b[i] != 0 && b[i] != (cell_t)(i + 1)) n++;
  return n;
}

static int manhattan(const cell_t *b)
{
  int i, d = 0;

  if (!md_ready) md_init();
  for (i = 0; i < MAX_BOARD; i++)
    d += md[b[i]][i];
  return d;
}

/*
 * Conflits linéaires sur une ligne (ou une colonne).
 *
 * `goal[i]` est la position que la tuile occupant la case i de la ligne doit
 * atteindre *dans cette même ligne*, ou -1 si sa case but est ailleurs (une
 * telle tuile devra de toute façon quitter la ligne, elle ne crée pas de
 * conflit).  Deux tuiles i < j sont en conflit si goal[i] > goal[j] : elles
 * sont dans le bon couloir mais dans le mauvais ordre, donc l'une des deux
 * devra sortir de la ligne puis y revenir — au moins 2 coups que la distance
 * de Manhattan ne compte pas, puisque ces 2 coups s'annulent.
 *
 * Il ne faut PAS ajouter 2 par paire en conflit : trois tuiles mutuellement
 * en conflit forment 3 paires mais une seule sortie suffit à en résoudre
 * deux, et +6 surestimerait. L'algorithme admissible (Hansson, Mayer & Yung)
 * retire donc gloutonnement la tuile qui participe au plus grand nombre de
 * conflits, compte +2, efface tous les conflits qu'elle portait, et
 * recommence.  On compte ainsi le nombre minimal de tuiles à faire sortir,
 * ce qui reste une borne inférieure du surcoût réel.
 */
static int line_conflicts(const int *goal, int n)
{
  int adj[WH_BOARD][WH_BOARD];
  int cnt[WH_BOARD];
  int i, j, extra = 0;

  memset(adj, 0, sizeof(adj));
  memset(cnt, 0, sizeof(cnt));

  for (i = 0; i < n; i++) {
    if (goal[i] < 0) continue;
    for (j = i + 1; j < n; j++) {
      if (goal[j] < 0) continue;
      if (goal[i] > goal[j]) {
        adj[i][j] = adj[j][i] = 1;
        cnt[i]++; cnt[j]++;
      }
    }
  }

  for (;;) {
    int best = -1;

    for (i = 0; i < n; i++)
      if (cnt[i] > 0 && (best < 0 || cnt[i] > cnt[best])) best = i;
    if (best < 0) break;

    for (j = 0; j < n; j++)
      if (adj[best][j]) { adj[best][j] = adj[j][best] = 0; cnt[j]--; }
    cnt[best] = 0;
    extra += 2;
  }

  return extra;
}

static int linear_conflict(const cell_t *b)
{
  int goal[WH_BOARD];
  int r, c, extra = 0;

  for (r = 0; r < WH_BOARD; r++) {
    for (c = 0; c < WH_BOARD; c++) {
      int v = b[r * WH_BOARD + c];
      goal[c] = (v != 0 && (v - 1) / WH_BOARD == r) ? (v - 1) % WH_BOARD : -1;
    }
    extra += line_conflicts(goal, WH_BOARD);
  }

  for (c = 0; c < WH_BOARD; c++) {
    for (r = 0; r < WH_BOARD; r++) {
      int v = b[r * WH_BOARD + c];
      goal[r] = (v != 0 && (v - 1) % WH_BOARD == c) ? (v - 1) / WH_BOARD : -1;
    }
    extra += line_conflicts(goal, WH_BOARD);
  }

  return extra;
}

int heuristic_eval(heuristic_id id, const cell_t *b)
{
  switch (id) {
    case H_ZERO:      return 0;
    case H_MISPLACED: return misplaced(b);
    case H_MANHATTAN: return manhattan(b);
    case H_LINEAR:    return manhattan(b) + linear_conflict(b);
    default:          return 0;
  }
}

const char *heuristic_name(heuristic_id id)
{
  switch (id) {
    case H_ZERO:      return "zero";
    case H_MISPLACED: return "misplaced";
    case H_MANHATTAN: return "manhattan";
    case H_LINEAR:    return "linear";
    default:          return "?";
  }
}

int heuristic_parse(const char *name, heuristic_id *out)
{
  heuristic_id id;

  for (id = 0; id < H_COUNT; id++) {
    if (strcmp(name, heuristic_name(id)) == 0) { *out = id; return 0; }
  }
  return -1;
}
