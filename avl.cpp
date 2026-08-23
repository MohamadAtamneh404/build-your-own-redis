#include "avl.h"
#include <algorithm>
#include <assert.h>

using namespace std;

static uint32_t max(uint32_t lhs, uint32_t rhs) {
  return lhs < rhs ? rhs : lhs;
}

static void avl_update(AVLNode *node) {
  node->height = 1 + max(avl_height(node->left), avl_height(node->right));
  node->cnt = 1 + avl_cnt(node->left) + avl_cnt(node->right);
}

static AVLNode *rot_left(AVLNode *node) {
  AVLNode *new_node = node->right;

  // Left Rotation (Fixes a heavy Right side)
  // Step 1: Y's left child becomes X's right child
  if (new_node->left) {
    new_node->left->parent = node;
  }
  node->right = new_node->left;

  // Step 2: X becomes Y's left child
  new_node->left = node;

  // Step 3: Y takes X's place as parent
  new_node->parent = node->parent;
  node->parent = new_node;

  // Update the heights and counts!
  avl_update(node);
  avl_update(new_node);

  return new_node;
}

static AVLNode *rot_right(AVLNode *node) {
  AVLNode *new_node = node->left;

  // Right Rotation (Fixes a heavy Left side)
  // Step 1: Y's RIGHT child becomes X's LEFT child
  if (new_node->right) {
    new_node->right->parent = node;
  }
  node->left = new_node->right;

  // Step 2: X becomes Y's RIGHT child
  new_node->right = node;

  // Step 3: Y takes X's place as parent
  new_node->parent = node->parent;
  node->parent = new_node;

  // Update the heights and counts!
  avl_update(node);
  avl_update(new_node);

  return new_node;
}

// The left subtree is too deep (Heavy Left side)
static AVLNode *avl_fix_left(AVLNode *node) {
  // If the left child's right side is heavier, a single Right Rotation isn't
  // enough! We first do a Left Rotation on the child (Transformation 2), then a
  // Right Rotation on the parent (Transformation 1)
  if (avl_height(node->left->left) < avl_height(node->left->right)) {
    node->left = rot_left(node->left);
  }
  return rot_right(node);
}

// The right subtree is too deep (Heavy Right side)
static AVLNode *avl_fix_right(AVLNode *node) {
  // If the right child's left side is heavier, a single Left Rotation isn't
  // enough! We first do a Right Rotation on the child, then a Left Rotation on
  // the parent.
  if (avl_height(node->right->right) < avl_height(node->right->left)) {
    node->right = rot_right(node->right);
  }
  return rot_left(node);
}

// fix imbalanced nodes and maintain invariants until the root is reached
AVLNode *avl_fix(AVLNode *node) {
  while (true) {
    AVLNode **from = &node; // save the fixed subtree here
    AVLNode *parent = node->parent;
    if (parent) {
      // attach the fixed subtree to the parent
      from = parent->left == node ? &parent->left : &parent->right;
    } // else: save to the local variable `node`
    // auxiliary data
    avl_update(node);
    // fix the height difference of 2
    uint32_t l = avl_height(node->left);
    uint32_t r = avl_height(node->right);
    if (l == r + 2) {
      *from = avl_fix_left(node);
    } else if (l + 2 == r) {
      *from = avl_fix_right(node);
    }
    // root node, stop
    if (!parent) {
      return *from;
    }
    // continue to the parent node because its height may be changed
    node = parent;
  }
}

// Detach a node that is "easy" to delete (it has 0 or 1 children).
// It returns the new root of the tree.
static AVLNode *avl_del_easy(AVLNode *node) {
  assert(!node->left || !node->right); // Ensure we don't have 2 children!

  // Grab the child (if it has one), otherwise this will be NULL
  AVLNode *child = node->left ? node->left : node->right;
  AVLNode *parent = node->parent;

  // Step 1: Tell the child who its new parent is
  if (child) {
    child->parent = parent;
  }

  // Step 2: Tell the parent who its new child is
  if (!parent) {
    return child; // We just deleted the absolute root of the tree!
  }
  AVLNode **from = parent->left == node ? &parent->left : &parent->right;
  *from = child;

  // Step 3: Now that the node is deleted, the tree might be unbalanced. Fix it!
  return avl_fix(parent);
}

// Detach any node from the tree and return the new root of the tree.
AVLNode *avl_del(AVLNode *node) {
  // If the node only has 0 or 1 children, we can use the easy deletion method!
  if (!node->left || !node->right) {
    return avl_del_easy(node);
  }

  // Hard Case: The node has 2 children. We cannot simply delete it, or the
  // children get orphaned! Solution: Find the "Successor" (the next largest
  // node). We will swap places with the successor.
  AVLNode *victim = node->right;
  while (victim->left) {
    victim = victim->left; // Keep going left to find the smallest node in the
                           // right subtree
  }

  // The successor is guaranteed to have at most 1 child, so we can use
  // `del_easy` to rip it out.
  AVLNode *root = avl_del_easy(victim);

  // Now we literally swap the victim into the exact spot where `node` was
  // sitting!
  *victim = *node; // Copy left, right, and parent pointers

  // Tell the children that they have a new parent now (the victim)
  if (victim->left) {
    victim->left->parent = victim;
  }
  if (victim->right) {
    victim->right->parent = victim;
  }

  // Tell the grandparent that it has a new child now (the victim)
  AVLNode **from = &root;
  AVLNode *parent = node->parent;
  if (parent) {
    from = parent->left == node ? &parent->left : &parent->right;
  }
  *from = victim;

  return root;
}

// Find a node by its rank (offset)
AVLNode *avl_offset(AVLNode *node, int64_t offset) {
  int64_t pos = 0; // relative to the starting node
  while (offset != pos) {
    if (pos < offset && node->right) {
      node = node->right;
      pos += avl_cnt(node->left) + 1;
    } else if (pos > offset && node->left) {
      node = node->left;
      pos -= avl_cnt(node->right) + 1;
    } else {
      AVLNode *parent = node->parent;
      if (!parent) {
        return NULL;
      }
      if (parent->right == node) {
        pos -= avl_cnt(node->left) + 1;
      } else {
        pos += avl_cnt(node->right) + 1;
      }
      node = parent;
    }
  }
  return node;
}
