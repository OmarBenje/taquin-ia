#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "search.h"
#include "board.h"
#include "list.h"

/*
 * Coût mémoire d'un nœud dans l'implémentation « list » : la structure Item
 * (qui porte déjà les chaînages prev/next, donc la liste n'ajoute rien) plus
 * le plateau alloué séparément par initBoard().
 */
#define ITEM_BYTES (sizeof(Item) + (size_t)MAX_BOARD)

/* ------------------------------------------------------------------ */
/* Reconstruction du chemin à partir des pointeurs parent               */
/* ------------------------------------------------------------------ */

static int move_between(int from_blank, int to_blank)
{
  int d = to_blank - from_blank;

  if (d == -WH_BOARD) return MOVE_UP;
  if (d ==  WH_BOARD) return MOVE_DOWN;
  if (d == -1)        return MOVE_LEFT;
  return MOVE_RIGHT;
}

static void extract_path(Item *goal, int *path, int cap, int *plen)
{
  Item *cur;
  int   len = 0, i;

  for (cur = goal; cur && cur->parent; cur = cur->parent) len++;
  *plen = len;
  if (!path) return;

  i = len - 1;
  for (cur = goal; cur && cur->parent; cur = cur->parent, i--) {
    if (i >= 0 && i < cap)
      path[i] = move_between((unsigned char)cur->parent->blank,
                             (unsigned char)cur->blank);
  }
}

/* ------------------------------------------------------------------ */
/* Implémentation « list » : BFS et A* sur les listes du squelette      */
/*                                                                      */
/* C'est le code d'origine, instrumenté et rien de plus. Il sert de      */
/* point de comparaison : toute optimisation ultérieure devra rendre la  */
/* même longueur de solution que celui-ci.                              */
/* ------------------------------------------------------------------ */

static int solve_list(const search_opts *o, const cell_t *start, stats_t *st,
                      int *path, int cap, int *plen)
{
  list_t openList, closedList;
  Item  *root, *cur, *child, *dup;
  long   live = 0;
  int    status = SEARCH_NOSOL;
  int    move;

  boardHeuristic(o->heuristic);

  initList(&openList);
  initList(&closedList);

  root = itemFromBoard(start);
  live++;
  st->generated++;
  st->bytes_peak = (size_t)live * ITEM_BYTES;
  addLast(&openList, root);
  st->open_max = 1;

  while (listCount(&openList) != 0) {

    cur = (o->algo == ALGO_BFS) ? popFirst(&openList) : popBest(&openList);

    if (puzzle_is_goal((const cell_t *)cur->board)) {
      extract_path(cur, path, cap, plen);
      st->solution_len = *plen;
      addLast(&closedList, cur);
      status = SEARCH_OK;
      break;
    }

    addLast(&closedList, cur);
    st->expanded++;

    for (move = 0; move < MAX_MOVES; move++) {
      child = getChildBoard(cur, move);
      if (child == NULL) continue;

      live++;
      st->generated++;
      if ((size_t)live * ITEM_BYTES > st->bytes_peak)
        st->bytes_peak = (size_t)live * ITEM_BYTES;

      /* Déjà fermé ? */
      dup = onList(&closedList, child->board);
      if (dup) {
        /* Avec une heuristique consistante, un nœud fermé a déjà son g
           optimal : ceci ne devrait jamais se déclencher. On le compte
           plutôt que de le supposer. */
        if (o->dup == DUP_GCOMPARE && child->g < dup->g) {
          dup->g = child->g; dup->h = child->h; dup->f = child->f;
          dup->depth = child->depth; dup->parent = child->parent;
          delList(&closedList, dup);
          addLast(&openList, dup);
          st->improved++;
        }
        st->duplicates++;
        freeItem(child); live--;
        continue;
      }

      /* Déjà sur l'open list ? */
      dup = onList(&openList, child->board);
      if (dup) {
        if (o->dup == DUP_GCOMPARE && child->g < dup->g) {
          dup->g = child->g; dup->h = child->h; dup->f = child->f;
          dup->depth = child->depth; dup->parent = child->parent;
          st->improved++;
        }
        st->duplicates++;
        freeItem(child); live--;
        continue;
      }

      addLast(&openList, child);
      if (listCount(&openList) > st->open_max)
        st->open_max = listCount(&openList);
    }

    if (o->max_nodes > 0 && st->generated >= o->max_nodes) {
      st->aborted = 1;
      status = SEARCH_ABORTED;
      break;
    }
  }

  st->closed_size = listCount(&closedList);
  cleanupList(&openList);
  cleanupList(&closedList);
  return status;
}

/* ------------------------------------------------------------------ */
/* Point d'entrée                                                       */
/* ------------------------------------------------------------------ */

int search_solve(const search_opts *o, const cell_t *start,
                 stats_t *st, int *path, int path_cap, int *path_len)
{
  double t0;
  int    len = 0, rc;

  stats_reset(st);
  t0 = now_seconds();

  switch (o->impl) {
    case IMPL_LIST:
    default:
      rc = solve_list(o, start, st, path, path_cap, &len);
      break;
  }

  st->seconds = now_seconds() - t0;
  if (path_len) *path_len = len;
  return rc;
}

/* ------------------------------------------------------------------ */
/* Noms et analyse des options                                          */
/* ------------------------------------------------------------------ */

const char *algo_name(algo_id id)
{
  switch (id) {
    case ALGO_BFS:   return "bfs";
    case ALGO_ASTAR: return "astar";
    default:         return "?";
  }
}

int algo_parse(const char *name, algo_id *out)
{
  algo_id id;

  for (id = 0; id < ALGO_COUNT; id++)
    if (strcmp(name, algo_name(id)) == 0) { *out = id; return 0; }
  return -1;
}

const char *impl_name(impl_id id)
{
  switch (id) {
    case IMPL_LIST: return "list";
    default:        return "?";
  }
}

int impl_parse(const char *name, impl_id *out)
{
  impl_id id;

  for (id = 0; id < IMPL_COUNT; id++)
    if (strcmp(name, impl_name(id)) == 0) { *out = id; return 0; }
  return -1;
}

const char *dup_name(dup_policy d)
{
  switch (d) {
    case DUP_NAIVE:    return "naive";
    case DUP_GCOMPARE: return "gcompare";
    default:           return "?";
  }
}

int dup_parse(const char *name, dup_policy *out)
{
  dup_policy d;

  for (d = 0; d < DUP_COUNT; d++)
    if (strcmp(name, dup_name(d)) == 0) { *out = d; return 0; }
  return -1;
}
