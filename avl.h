#pragma once

#include <stddef.h>
#include <stdint.h>

struct AVLNode {
  AVLNode *parent = NULL; // added
  AVLNode *left = NULL;
  AVLNode *right = NULL;
  uint32_t height = 0; // auxiliary data for AVL tree
  uint64_t cnt = 0;    // number of nodes in the subtree rooted at this node
};

inline void avl_init(AVLNode *node) {
  node->left = node->right = node->parent = NULL;
  node->height = 1;
  node->cnt = 1;
}

static uint32_t avl_height(AVLNode *node) { return node ? node->height : 0; }

static uint64_t avl_cnt(AVLNode *node) { return node ? node->cnt : 0; }

// API
AVLNode *avl_fix(AVLNode *node);
AVLNode *avl_del(AVLNode *node);
