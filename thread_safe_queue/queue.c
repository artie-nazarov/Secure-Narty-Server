// Concurrent Queue ADT implementation

#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// queue has FIFO properties and should support multi producer and multi consumer
struct queue {
    void **queue;
    int size;
    int head, tail;
    int elems_enqueued;

    // pthread variables
    pthread_mutex_t mutex;
    pthread_cond_t cond_full;
    pthread_cond_t cond_empty;
};

// Allocates a queue with size `size`
queue_t *queue_new(int size) {
    queue_t *Q = (queue_t *) calloc(1, sizeof(queue_t));
    if (Q == NULL) {
        return NULL;
    }
    Q->queue = (void **) malloc(size * sizeof(void *));
    if (Q->queue == NULL) {
        free(Q);
        Q = NULL;
        return NULL;
    }
    Q->size = size;
    Q->head = 0;
    Q->tail = size - 1;
    Q->elems_enqueued = 0;

    // pthread variables
    if(pthread_mutex_init(&Q->mutex, NULL)) {
        queue_delete(&Q);
        return NULL;
    }
    if(pthread_cond_init(&Q->cond_full, NULL)) {
        queue_delete(&Q);
        return NULL;
    }
    if(pthread_cond_init(&Q->cond_empty, NULL)) {
        queue_delete(&Q);
        return NULL;
    }

    return Q;
}

// frees a queue (should assume the queue is empty)
void queue_delete(queue_t **q) {
    if (q != NULL && *q != NULL) {
        queue_t *Q = *q;
        if (Q->queue != NULL) {
            free(Q->queue);
            Q->queue = NULL;
        }
        pthread_mutex_destroy(&Q->mutex);
        pthread_cond_destroy(&Q->cond_full);
        pthread_cond_destroy(&Q->cond_empty);
        
        free(*q);
    }
    *q = NULL;
    
    return;
}

// Private Helper Functionality
// Check if a queue is empty
bool is_empty(queue_t *q) {
    return q->elems_enqueued == 0;
}
// Check if a queue is full
bool is_full(queue_t *q) {
    return (q->elems_enqueued == q->size);
}

// add an element to the queue
// blocks if queue is full
// adds an element to the back of the queue (tail)
bool queue_push(queue_t *q, void *elem) {
    if (!q) {
        return false;
    }
    // Acquire a lock
    pthread_mutex_lock(&q->mutex);
    // If the queue is full wait for another thread to dequeue an element
    while (is_full(q)) {
        pthread_cond_wait(&q->cond_full, &q->mutex);
    }
    // Perform queue push
    q->tail = (q->tail + 1) % q->size;
    q->queue[q->tail] = elem;
    q->elems_enqueued += 1;

    // Check if a message needs to be broadcasted to potentially hanging threads trying to pop()
    if (q->elems_enqueued == 1) {
        // Notify threads waiting to dequeue that the queue is no longer empty
        pthread_cond_broadcast(&q->cond_empty);
    }
    // Release the lock
    pthread_mutex_unlock(&q->mutex);
    
    return true;
}

// remove an element from the queue
// blocks if the queue is empty
bool queue_pop(queue_t *q, void **elem) {
    if (!q) {
        return false;
    }
    // Acquire a lock
    pthread_mutex_lock(&q->mutex);
    // If the queue is empty wait for another thread to enqueue an element
    while (is_empty(q)) {
        pthread_cond_wait(&q->cond_empty, &q->mutex);
    }
    // Perform queue pop
    *elem = q->queue[q->head];
    q->head = (q->head + 1) % q->size;
    q->elems_enqueued -= 1;

    // Check if a message needs to be broadcasted to potentially hanging threads trying to push()
    if  (q->elems_enqueued + 1 == q->size) {
        // Notify threads waiting to enqueue that the queue is no longer full
        pthread_cond_broadcast(&q->cond_full);
    }
    // Release the lock
    pthread_mutex_unlock(&q->mutex);

    return true;
}
