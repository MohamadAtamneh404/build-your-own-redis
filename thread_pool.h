#pragma once

#include <deque>
#include <pthread.h>
#include <stddef.h>
#include <vector>

// Represents a single "Job" or "Task" to be done in the background.
struct Work {
  void (*f)(void *) = NULL; // A pointer to the function we want to run (e.g., cb_destroy)
  void *arg = NULL;         // The argument to pass to that function (e.g., the pointer to the Entry)
};

// The ThreadPool structure holds all the state for our background workers.
struct ThreadPool {
  std::vector<pthread_t> threads; // An array keeping track of all the background worker threads we've spawned
  std::deque<Work> queue;         // The actual "line" or "queue" where the main thread drops off Work boxes
  pthread_mutex_t mu;             // A lock! Only one thread can touch the queue at a time so they don't corrupt memory
  pthread_cond_t not_empty;       // A condition variable. This is like a bell that rings to wake up sleeping threads when a new job arrives!
};

// Spawns the background threads and gets them ready to work
void thread_pool_init(ThreadPool *tp, size_t num_threads);

// The function the main event loop calls to drop off a new job in the queue
void thread_pool_queue(ThreadPool *tp, void (*f)(void *), void *arg);
