#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "search.h"
#include "board.h"
#include "list.h"
#include "node.h"
#include "pqueue.h"
#include "hashset.h"

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
/* File FIFO circulaire, pour le BFS de l'implémentation rapide         */
/* ------------------------------------------------------------------ */

typedef struct {
  node_t **v;
  size_t   head, n, cap, bytes;
} fifo_t;

static void fifo_init(fifo_t *q)
{
  q->v = NULL;
  q->head = q->n = q->cap = q->bytes = 0;
}

static void fifo_free(fifo_t *q)
{
  free(q->v);
  fifo_init(q);
}

static void fifo_push(fifo_t *q, node_t *x)
{
  if (q->n == q->cap) {
    size_t cap = q->cap ? q->cap * 2 : 1024, i;
    node_t **v = malloc(cap * sizeof(*v));

    for (i = 0; i < q->n; i++) v[i] = q->v[(q->head + i) % q->cap];
    free(q->v);
    q->v = v; q->cap = cap; q->head = 0;
    q->bytes = cap * sizeof(*v);
  }
  q->v[(q->head + q->n) % q->cap] = x;
  q->n++;
}

static node_t *fifo_pop(fifo_t *q)
{
  node_t *x;

  if (q->n == 0) return NULL;
  x = q->v[q->head];
  q->head = (q->head + 1) % q->cap;
  q->n--;
  return x;
}

/* ------------------------------------------------------------------ */
/* Implémentation « fast » : tas binaire + table de hachage             */
/*                                                                      */
/* Mêmes règles, mêmes politiques de doublons que solve_list ; seules   */
/* les structures de données changent.                                  */
/* ------------------------------------------------------------------ */

static void node_reset(node_t *n, const cell_t *board, int blank,
                       int g, int h, node_t *parent)
{
  memcpy(n->board, board, MAX_BOARD);
  n->parent   = parent;
  n->hnext    = NULL;
  n->g        = g;
  n->h        = h;
  n->heap_idx = -1;
  n->blank    = (unsigned char)blank;
  n->closed   = 0;
}

static void extract_path_nodes(node_t *goal, int *path, int cap, int *plen)
{
  node_t *cur;
  int     len = 0, i;

  for (cur = goal; cur && cur->parent; cur = cur->parent) len++;
  *plen = len;
  if (!path) return;

  i = len - 1;
  for (cur = goal; cur && cur->parent; cur = cur->parent, i--)
    if (i >= 0 && i < cap)
      path[i] = move_between(cur->parent->blank, cur->blank);
}

static int solve_fast(const search_opts *o, const cell_t *start, stats_t *st,
                      int *path, int cap, int *plen)
{
  arena_t arena;
  heap_t  open;
  fifo_t  queue;
  hset_t  seen;
  node_t *root, *cur;
  int     status = SEARCH_NOSOL;
  int     use_heap = (o->algo != ALGO_BFS);

  arena_init(&arena);
  hset_init(&seen);
  heap_init(&open);
  fifo_init(&queue);

  root = arena_new(&arena);
  node_reset(root, start, puzzle_blank(start), 0,
             use_heap ? heuristic_eval(o->heuristic, start) : 0, NULL);
  hset_insert(&seen, root);
  st->generated = 1;
  if (use_heap) heap_push(&open, root); else fifo_push(&queue, root);
  st->open_max = 1;

  for (;;) {
    size_t open_n;
    int    move;

    cur = use_heap ? heap_pop(&open) : fifo_pop(&queue);
    if (!cur) break;

    if (puzzle_is_goal(cur->board)) {
      extract_path_nodes(cur, path, cap, plen);
      st->solution_len = *plen;
      status = SEARCH_OK;
      break;
    }

    cur->closed = 1;
    st->expanded++;

    for (move = 0; move < MAX_MOVES; move++) {
      cell_t  next[MAX_BOARD];
      node_t *dup, *child;
      int     nb, newg;

      nb = puzzle_apply(cur->board, cur->blank, move, next);
      if (nb < 0) continue;

      st->generated++;
      newg = cur->g + 1;

      dup = hset_find(&seen, next);
      if (dup) {
        st->duplicates++;

        /* Le BFS trouve toujours le plus court chemin en premier : il n'y a
           rien à améliorer. Pour A*, voir README question 3. */
        if (use_heap && o->dup == DUP_GCOMPARE && newg < dup->g) {
          dup->g      = newg;
          dup->parent = cur;
          st->improved++;
          if (dup->heap_idx >= 0) {
            heap_decrease(&open, dup);
          } else if (dup->closed) {
            /* Réouverture. Avec une heuristique consistante, ceci ne devrait
               jamais se produire ; le compteur `improved` le prouve ou non. */
            dup->closed = 0;
            heap_push(&open, dup);
          }
        }
        continue;
      }

      child = arena_new(&arena);
      node_reset(child, next, nb, newg,
                 use_heap ? heuristic_eval(o->heuristic, next) : 0, cur);
      hset_insert(&seen, child);
      if (use_heap) heap_push(&open, child); else fifo_push(&queue, child);
    }

    open_n = use_heap ? heap_size(&open) : queue.n;
    if ((long)open_n > st->open_max) st->open_max = (long)open_n;

    if (o->max_nodes > 0 && st->generated >= o->max_nodes) {
      st->aborted = 1;
      status = SEARCH_ABORTED;
      break;
    }
  }

  st->closed_size  = (long)seen.count;
  st->hash_probes  = hset_avg_probes(&seen);
  /* Les trois structures ne rendent jamais de mémoire en cours de route :
     leur taille finale est donc leur pic. */
  st->bytes_peak   = arena.bytes + open.bytes + queue.bytes + seen.bytes;

  arena_free(&arena);
  heap_free(&open);
  fifo_free(&queue);
  hset_free(&seen);
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
    case IMPL_FAST:
      rc = solve_fast(o, start, st, path, path_cap, &len);
      break;
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
    case IMPL_FAST: return "fast";
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
