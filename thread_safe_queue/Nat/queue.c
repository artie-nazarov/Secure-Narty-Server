#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include "queue.h"

typedef struct queue {
    void **elems;
    int head, tail, num_entries, size;
    pthread_mutex_t do_work;
    pthread_cond_t not_full, not_empty;
} queue_t;

/**
 * Queue constructor - allocates the required memory and initialize necessary variables
 * @param size Max size (aka capacity) of elements queue can hold
 * @return The new queue object
 * */
queue_t *queue_new(int size) {
    queue_t *q = (queue_t *) malloc(sizeof(queue_t));
    if (!q) {
        return NULL;
    }
    q->elems = (void *) malloc(size * sizeof(void*));
    if (!q->elems) {
        free(q);
        q = NULL;
        return NULL;
    }
    q->head = 0;
    q->tail = 0;
    q->num_entries = 0;
    q->size = size;
    pthread_mutex_init(&q->do_work, NULL);
    pthread_cond_init(&q->not_full, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    return q;
}

/**
 * Queue destructor - free any allocated memory and set the passed in pointer to NULL.
 * @param q A pointer to the queue objust to be deleted 
 * */
void queue_delete(queue_t **q) {
    if (q) {
        pthread_mutex_destroy(&(*q)->do_work);
        pthread_cond_destroy(&(*q)->not_full);
        pthread_cond_destroy(&(*q)->not_empty);
        free((*q)->elems);
        free(*q);
    }
}

/**
 * Enqueue - adds elem to queue q, observing FIFO, First In First Out, semantics
 * for thread-safety, push should:
 *      1. allow multiple concurrent writers (aka callers)
 *      2. block write until no other threads are reading OR writing
 *      3. block enqueue if the queue is full
 * @param q pointer to queue to be pushed to
 * @param p void pointer to elem to be pushed
 * @return boolean true unless given an invalid pointer
 * */
bool queue_push(queue_t *q, void *elem) {
    if (!&(q) || !&(elem)) {
        return false;
    }
    pthread_mutex_lock(&q->do_work);
    while (q->num_entries == q->size) {
        pthread_cond_wait(&q->not_full, &q->do_work);
    }
    q->num_entries++;
    q->elems[q->tail] = elem;
    q->tail = (q->tail + 1) % q->size;
    // TODO: only signal if num_entries = 1
    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->do_work);
    return true;
}

/**
 * Dequeue - removes an elem from queue q, observing FIFO, First In First Out, semantics
 * for thread-safety, pop should:
 *      1. allow multiple concurrent readers
 *      2. block if the queue is empty
 * @param q pointer to queue to pop from
 * @param p pointer to memory address to store popped element
 * @return boolean true unless given an invalid pointer
 * */
bool queue_pop(queue_t *q, void **elem) {
    if (!&(q) || !&(elem)) {
        return false;
    }
    pthread_mutex_lock(&q->do_work);
    while (q->num_entries == 0) {
        pthread_cond_wait(&q->not_empty, &q->do_work);
    }
    q->num_entries--;
    *elem = q->elems[q->head];
    q->head = (q->head + 1) % q->size;
    // TODO: only signal if num_entries = size -1 
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->do_work);
    return true;
}
