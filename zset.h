#pragma once

#include "avl.h"
#include "hashtable.h"
#include <string>

// To use the `container_of` trick, we must define it here if it's not globally available.
#ifndef container_of
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

// A single Player in the leaderboard.
struct ZNode {
    AVLNode tree_node;  // Embedded AVL Node (used to sort by score)
    HNode hmap_node;    // Embedded Hashtable Node (used to lookup by name)
    double score = 0;
    std::string name;
};

// The ZSet contains BOTH the AVL Tree and the Hashtable.
struct ZSet {
    AVLNode *tree = NULL;
    HMap hmap;
};

// Adds a new node, or updates the score of an existing node.
// Returns true if a new node was created, false if it just updated an existing node.
bool zset_add(ZSet *zset, const char *name, size_t len, double score);

// Instantly finds a node by its name using the Hashtable.
ZNode *zset_lookup(ZSet *zset, const char *name, size_t len);

// Removes a node from both the tree and the hashtable.
ZNode *zset_pop(ZSet *zset, const char *name, size_t len);

// Used to query a range of scores (Find the first person with `score`, and skip `offset` people).
ZNode *zset_query(ZSet *zset, double score, const char *name, size_t len, int64_t offset);

// Frees all memory associated with the ZSet.
void zset_dispose(ZSet *zset);
