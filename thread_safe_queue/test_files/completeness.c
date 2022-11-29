#include "queue.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define POP_PUSHES 1000
#define QUEUE_SIZE 10
#define SOME_VAL_A 2

void *threadA(void *args) {
    queue_t *q = args;

    // push first 500 elements
    for (int i = 0; i < POP_PUSHES / 2; i++) {
        if (!queue_push(q, (void *) (intptr_t) 1)) {
            return (void *) 1;
        }
    }
    // Enqueue some value A
    if (!queue_push(q, (void *) (intptr_t) SOME_VAL_A)) {
        return (void *) 1;
    }
    // push the remaining 500 elements
    for (int i = 0; i < POP_PUSHES / 2; i++) {
        if (!queue_push(q, (void *) (intptr_t) 1)) {
            return (void *) 1;
        }
    }

    return NULL;
}

void *threadB(void *args) {
    queue_t *q = args;

    bool validated = false;
    void *rv;
    for (int i = 0; i < POP_PUSHES; i++) {
        if (!queue_pop(q, &rv)) {
            return (void *) 1;
        }
        if ((int) rv == SOME_VAL_A) {
            validated = true;
        }
    }

    return (void *) validated;
}

/** Test 3 - Completeness
 * If a thread T1 enqueues item a, some consumer T2 will dequeue a after a finite number of dequeues.
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

        // Launch threads A and B. tA will push 1's and some checker value A, tB will pop elements off the queue and check that the value A hasn't been lost
        pthread_create(&tA, NULL, threadA, (void *) q);
        pthread_create(&tB, NULL, threadB, (void *) q);
        void *rc;

        pthread_join(tA, &rc);
        if (rc != NULL)
            return 1;
        pthread_join(tB, &rc);
        if ((bool) rc != true)
            return 1;

        queue_delete(&q);
    }

    return 0;
}
