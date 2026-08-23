#include "zset.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

// A tiny hashing algorithm just for strings (same as server.cpp)
static uint64_t str_hash(const uint8_t *data, size_t len) {
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x01000193;
    }
    return h;
}

// -----------------------------------------------------------------------------
// HASHTABLE CALLBACK
// -----------------------------------------------------------------------------
// The Hashtable needs a way to check if two nodes have the exact same name.
static bool znode_eq(HNode *lhs, HNode *rhs) {
    // 1. Cast the raw HNodes back into their parent ZNodes
    ZNode *le = container_of(lhs, ZNode, hmap_node);
    ZNode *re = container_of(rhs, ZNode, hmap_node);
    // 2. Compare their names
    return le->name == re->name;
}

// -----------------------------------------------------------------------------
// AVL TREE CALLBACK
// -----------------------------------------------------------------------------
// The AVL Tree needs a way to check if Node A is "less than" Node B.
// In a ZSet, we sort by Score FIRST. If they have a tie, we sort by Name alphabetically.
static bool znode_less(AVLNode *lhs, AVLNode *rhs) {
    ZNode *le = container_of(lhs, ZNode, tree_node);
    ZNode *re = container_of(rhs, ZNode, tree_node);
    if (le->score != re->score) {
        return le->score < re->score;
    }
    return le->name < re->name;
}


// -----------------------------------------------------------------------------
// ZSET API
// -----------------------------------------------------------------------------

// Instantly find a player by their name using the Hashtable
ZNode *zset_lookup(ZSet *zset, const char *name, size_t len) {
    ZNode dummy;
    dummy.name.assign(name, len);
    dummy.hmap_node.hcode = str_hash((uint8_t *)name, len);
    
    // O(1) Lookup!
    HNode *found = hm_lookup(&zset->hmap, &dummy.hmap_node, &znode_eq);
    return found ? container_of(found, ZNode, hmap_node) : NULL;
}

// A helper function to insert a ZNode into the AVL Tree
static void tree_add(ZSet *zset, ZNode *node) {
    if (!zset->tree) {
        zset->tree = &node->tree_node;
        return;
    }
    AVLNode *cur = zset->tree;
    while (true) {
        // Do we go Left or Right? Use `znode_less` to decide!
        AVLNode **from = znode_less(&node->tree_node, cur) ? &cur->left : &cur->right;
        if (!*from) {
            *from = &node->tree_node;
            node->tree_node.parent = cur;
            // Balance the tree on the way back up!
            zset->tree = avl_fix(&node->tree_node);
            break;
        }
        cur = *from;
    }
}

// A helper function to update a player's score
static void zset_update(ZSet *zset, ZNode *node, double score) {
    if (node->score == score) {
        return; // Nothing changed
    }
    // You cannot simply change a node's score while it is inside the tree, 
    // because it will break the sorting! You MUST delete it, update it, and re-insert it.
    zset->tree = avl_del(&node->tree_node);
    node->score = score;
    avl_init(&node->tree_node);
    tree_add(zset, node);
}

// Add a new player, or update their score if they exist
bool zset_add(ZSet *zset, const char *name, size_t len, double score) {
    // 1. Does this player already exist?
    ZNode *node = zset_lookup(zset, name, len);
    if (node) {
        // Yes! Just update their score.
        zset_update(zset, node, score);
        return false; // False = not a new node
    }
    
    // 2. They don't exist. Create a brand new node!
    node = new ZNode;
    node->name.assign(name, len);
    node->score = score;
    
    // 3. Initialize the embedded nodes
    avl_init(&node->tree_node);
    node->hmap_node.next = NULL;
    node->hmap_node.hcode = str_hash((uint8_t *)name, len);
    
    // 4. Insert into BOTH data structures!
    hm_insert(&zset->hmap, &node->hmap_node);
    tree_add(zset, node);
    
    return true; // True = created a new node
}

// Remove a player from both the Hashtable and the AVL Tree
ZNode *zset_pop(ZSet *zset, const char *name, size_t len) {
    ZNode dummy;
    dummy.name.assign(name, len);
    dummy.hmap_node.hcode = str_hash((uint8_t *)name, len);
    
    // Delete from Hashtable
    HNode *found = hm_delete(&zset->hmap, &dummy.hmap_node, &znode_eq);
    if (!found) {
        return NULL;
    }
    ZNode *node = container_of(found, ZNode, hmap_node);
    
    // Delete from AVL Tree
    zset->tree = avl_del(&node->tree_node);
    return node;
}

// Query the AVL tree to find a specific score, and then skip `offset` places
ZNode *zset_query(ZSet *zset, double score, const char *name, size_t len, int64_t offset) {
    ZNode dummy;
    dummy.score = score;
    dummy.name.assign(name, len);
    
    AVLNode *found = NULL;
    AVLNode *cur = zset->tree;
    while (cur) {
        if (znode_less(cur, &dummy.tree_node)) {
            cur = cur->right;
        } else {
            found = cur; // This might be a match! Keep searching left to find the VERY first one.
            cur = cur->left;
        }
    }
    
    if (found) {
        found = avl_offset(found, offset);
    }
    return found ? container_of(found, ZNode, tree_node) : NULL;
}

// Recursively delete all nodes in the tree
static void tree_dispose(AVLNode *node) {
    if (!node) return;
    tree_dispose(node->left);
    tree_dispose(node->right);
    delete container_of(node, ZNode, tree_node);
}

// Destroy the entire ZSet
void zset_dispose(ZSet *zset) {
    tree_dispose(zset->tree);
    hm_clear(&zset->hmap);
}
