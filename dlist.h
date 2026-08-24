#pragma once

struct DList {
  struct DList *prev;
  struct DList *next;
};

static void d_list_init(struct DList *node) { node->prev = node->next = node; }

// 1. Check if the list is empty
static bool dlist_empty(struct DList *node) { return node->next == node; }

// 2. Remove a node from the list
static void dlist_detach(struct DList *node) {
  struct DList *prev = node->prev;
  struct DList *next = node->next;

  // Wire the neighbors to each other, cutting `node` out!
  prev->next = next;
  next->prev = prev;

  // Reset the detached node so it doesn't have dangling pointers
  d_list_init(node);
}

// 3. Insert a rookie node directly before a target node(in the back in last)
static void dlist_insert_before(struct DList *target, struct DList *rookie) {
  struct DList *prev = target->prev;

  // Wire the rookie to its new neighbors
  rookie->prev = prev;
  rookie->next = target;

  // Wire the neighbors to the rookie
  prev->next = rookie;
  target->prev = rookie;
}
