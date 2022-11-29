#include "queue.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define POP_PUSHES 1000
#define QUEUE_SIZE 200

void *threadA(void *args) {
    queue_t *q = args;

    for (int i = 0; i < POP_PUSHES; i++) {
        if (!queue_push(q, (void *) (intptr_t) 1)) {
            return (void *) 1;
        }
    }

    return NULL;
}

void *threadB(void *args) {
    queue_t *q = args;

    void *rv;
    for (int i = 0; i < POP_PUSHES; i++) {
        if (!queue_pop(q, &rv) || (int) rv != 1) {
            return (void *) 1;
        }
    }

    return NULL;
}

/** Test 2 - Validity
 * If a consumer thread T2 dequeues some item a, then a was enqueued by some thread T1. Intuitively, no consumers may read junk.
*/
int main(int argc, char **argv) {
    if (argc != 2) {
        return 2;
    }

    int iters = atoi(argv[1]);

    for (int i = 0; i < iters; i++) {
        queue_t *q = queue_new(QUEUE_SIZE);
        if (q == NULL) {
            return 1;
        }
        pthread_t tA, tB;

        // Launch threads A and B. tA will push 1's onto the queue, tB will pop elements off the queue
        pthread_create(&tA, NULL, threadA, (void *) q);
        pthread_create(&tB, NULL, threadB, (void *) q);
        void *rc;
        pthread_join(tA, &rc);
        if (rc != NULL)
            return 1;
        pthread_join(tB, &rc);
        if (rc != NULL)
            return 1;

        queue_delete(&q);
    }

    return 0;
}
