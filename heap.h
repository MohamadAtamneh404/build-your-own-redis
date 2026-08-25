#pragma once
#include <stddef.h>
#include <stdint.h>

struct HeapItem {
    uint64_t val; // The expiration time in microseconds
    size_t *ref;  // A pointer to the key's internal `heap_idx` variable
};

// Whenever a key's TTL is added, removed, or changed, this function
// bubbles it up or down the tree to keep the Min-Heap perfectly sorted!
void heap_update(HeapItem *a, size_t pos, size_t len);
