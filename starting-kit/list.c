#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "list.h"

/* Item Functions */

Item *nodeAlloc()
{
  Item *node;

  node = (Item *)malloc(sizeof(Item));
  assert(node);

  node->board = NULL;
  node->parent = NULL;
  node->prev = NULL;
  node->next = NULL;
  node->f = node->g = node->h = 0.0f;

  return node;
}

void freeItem(Item *node)
{
  if (node && node->board)
    free(node->board);
  if (node)
    free(node);
}

void initList(list_t *list_p)
{
  // if (list_p == NULL) list_p = malloc(sizeof(list_t));
  assert(list_p);

  list_p->numElements = 0;
  list_p->first = NULL;
  list_p->last = NULL;
}

int listCount(list_t *list)
{
  return list->numElements;
}

// return an item with corresponding board , or null
Item *onList(list_t *list, char *board)
{
  Item *item = list->first;
  while (item)
  {
    if (item->board && memcmp(item->board, board, (unsigned char)item->size) == 0)
      return item;
    item = item->next;
  }
  return NULL;
}

// return and remove first item
Item *popFirst(list_t *list)
{
  if (!list->first)
    return NULL;

  Item *item = list->first;
  list->first = item->next;
  if (list->first)
    list->first->prev = NULL;
  else
    list->last = NULL;

  item->prev = item->next = NULL;
  list->numElements--;
  return item;
}

// return and remove last item
Item *popLast(list_t *list)
{
  if (!list->last)
    return NULL;

  Item *item = list->last;
  list->last = item->prev;
  if (list->last)
    list->last->next = NULL;
  else
    list->first = NULL;

  item->prev = item->next = NULL;
  list->numElements--;
  return item;
}

// remove a node from list
void delList(list_t *list, Item *node)
{
  if (!node)
    return;

  if (node->prev)
    node->prev->next = node->next;
  else
    list->first = node->next;

  if (node->next)
    node->next->prev = node->prev;
  else
    list->last = node->prev;

  node->prev = node->next = NULL;
  list->numElements--;
}

// return and remove best item with minimal f value
Item *popBest(list_t *list)
{
  if (!list->first)
    return NULL;

  Item *best = list->first;
  Item *cur = list->first->next;
  while (cur)
  {
    if (cur->f < best->f)
      best = cur;
    cur = cur->next;
  }

  delList(list, best);
  return best;
}

// add item in top
void addFirst(list_t *list, Item *node)
{
  if (!node)
    return;

  node->prev = NULL;
  node->next = list->first;
  if (list->first)
    list->first->prev = node;
  else
    list->last = node;

  list->first = node;
  list->numElements++;
}

// add item in queue
void addLast(list_t *list, Item *node)
{
  if (!node)
    return;

  node->next = NULL;
  node->prev = list->last;
  if (list->last)
    list->last->next = node;
  else
    list->first = node;

  list->last = node;
  list->numElements++;
}

void cleanupList(list_t *list)
{
  Item *item = list->first;
  while (item)
  {
    Item *next = item->next;
    freeItem(item);
    item = next;
  }
  list->first = NULL;
  list->last = NULL;
  list->numElements = 0;
}

void printList(list_t list)
{
  Item *item = list.first;
  while (item)
  {
    printf("%.2f [%s] - ", item->f, item->board);
    item = item->next;
  }
  printf(" (nb_items: %d)\n", list.numElements);
}

// TEST LIST — compiler avec : gcc -Wall -o list_test list.c -DTEST_LIST
#ifdef TEST_LIST
int main()
{
  Item *item, *node;
  char str[3];

  list_t openList;

  initList(&openList);

  for (int i = 0; i < 10; i++)
  {
    item = nodeAlloc();
    item->f = i;
    sprintf(str, "%2d", i);
    item->board = strdup(str);
    item->size = 3;
    addLast(&openList, item);
  }

  printList(openList);
  printf("\n");

  for (int i = 20; i < 25; i++)
  {
    item = nodeAlloc();
    item->f = i;
    item->size = 3;
    sprintf(str, "%2d", i);
    item->board = strdup(str);
    addFirst(&openList, item);
  }

  printList(openList);
  printf("\n");

  node = popFirst(&openList);
  if (node)
    printf("first: %.2f\n", node->f);
  printList(openList);
  printf("\n");

  node = popLast(&openList);
  if (node)
    printf("last: %.2f\n", node->f);
  printList(openList);
  printf("\n");

  node = popBest(&openList);
  printf("best node = %.2f\n", node->f);
  printList(openList);
  printf("\n");

  strcpy(str, "23");
  node = onList(&openList, str);
  if (node)
    printf("found %s: %.2f!\n", str, node->f);
  printList(openList);
  printf("\n");

  node = popFirst(&openList);

  item = nodeAlloc();
  item->f = 50;
  item->size = 3;
  sprintf(str, "50");
  item->board = strdup(str);
  addLast(&openList, item);

  printf("clean\n");
  cleanupList(&openList);
  printList(openList);
  printf("\n");

  return 0;
}
#endif /* TEST_LIST */
