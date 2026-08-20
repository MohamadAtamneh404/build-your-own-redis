#pragma once

#include <stddef.h>
#include <stdint.h>

// hashtable node, should be embedded into the payload
struct HNode {
  HNode *next = NULL;
  uint64_t hcode = 0;
};

// a simple fixed-sized hashtable
struct HTab {
  HNode **tab = NULL; // array of slots
  size_t mask = 0;    // power of 2 array size, 2^n - 1 ,if we have 8 buckets
                      // mask=7 to calculate position in the array hcode & mask
  size_t size = 0;    // number of keys
};

// the real hashtable interface.
// it uses 2 hashtables for progressive rehashing.
struct HMap {
  HTab newer;
  HTab older;
  size_t migrate_pos = 0; // Because the server moves users from older to newer
                          // a little bit at a time
  // , it needs to remember where it left off.
  // If it just finished moving the users out of Bucket #5,
  //  migrate_pos is updated to 6 so it knows exactly where to start next time.
};

HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
void hm_insert(HMap *hmap, HNode *node);
HNode *hm_delete(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
void hm_clear(HMap *hmap);
size_t hm_size(HMap *hmap);
