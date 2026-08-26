#include <assert.h>
#include "thread_pool.h"

// This is the infinite loop that every background worker thread runs!
static void *worker(void *arg) {
    ThreadPool *tp = (ThreadPool *)arg; // The thread gets a pointer to the global ThreadPool
    
    while (true) {
        // Step 1: Grab the lock! A thread MUST hold the lock before touching the queue.
        pthread_mutex_lock(&tp->mu);
        
        // Step 2: If the queue is empty, go to sleep!
        // We use a while loop to handle "spurious wakeups" (a quirk where threads randomly wake up)
        while (tp->queue.empty()) {
            // pthread_cond_wait puts the thread to sleep AND unlocks the mutex at the same time.
            // When the thread is woken up by the bell, it automatically re-locks the mutex!
            pthread_cond_wait(&tp->not_empty, &tp->mu);
        }

        // Step 3: We are awake, we have the lock, and the queue is NOT empty!
        Work w = tp->queue.front(); // Grab the very first job in the line
        tp->queue.pop_front();      // Remove it from the line
        
        // Step 4: Unlock the mutex! We have our job, let other threads grab jobs too.
        pthread_mutex_unlock(&tp->mu);

        // Step 5: Actually execute the heavy work! (e.g. deleting 10 million ZSET nodes)
        w.f(w.arg);
    }
    return NULL; // This line is never reached since the thread runs forever
}

// Sets up the ThreadPool before the server starts
void thread_pool_init(ThreadPool *tp, size_t num_threads) {
    assert(num_threads > 0);

    // Initialize the lock (mutex) and the bell (condition variable)
    int rv = pthread_mutex_init(&tp->mu, NULL);
    assert(rv == 0);
    rv = pthread_cond_init(&tp->not_empty, NULL);
    assert(rv == 0);

    // Spawn the requested number of background threads (e.g., 4)
    tp->threads.resize(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        // pthread_create starts a new thread. We tell it to run the `worker` function above!
        int rv = pthread_create(&tp->threads[i], NULL, &worker, tp);
        assert(rv == 0);
    }
}

// Used by the main event loop to package a job and ring the bell!
void thread_pool_queue(ThreadPool *tp, void (*f)(void *), void *arg) {
    // Step 1: Grab the lock so we don't interfere with sleeping/waking workers
    pthread_mutex_lock(&tp->mu);
    
    // Step 2: Push the new job to the very back of the line
    tp->queue.push_back(Work {f, arg});
    
    // Step 3: Ring the bell! This wakes up exactly ONE sleeping worker thread
    pthread_cond_signal(&tp->not_empty);
    
    // Step 4: Unlock the mutex so the woken-up worker can grab the job
    pthread_mutex_unlock(&tp->mu);
}
