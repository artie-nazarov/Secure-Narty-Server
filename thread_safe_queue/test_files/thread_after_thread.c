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
    if (!queue_push(q, (void *) (intptr_t) 1)) {
        return (void *) 1;
    }

    return NULL;
}

void *threadB(void *args) {
    queue_t *q = args;
    if (!queue_push(q, (void *) (intptr_t) 2)) {
        return (void *) 1;
    }

    return NULL;
}

void *threadC(void *args) {
    queue_t *q = args;

    void *rv;
    if (!queue_pop(q, &rv) || (int) rv != 1) {
        return (void *) 1;
    }

    return NULL;
}

void *threadD(void *args) {
    queue_t *q = args;

    void *rv;
    if (!queue_pop(q, &rv) || (int) rv != 2) {
        return (void *) 1;
    }

    return NULL;
}

/** Test 9
 * Tests the following scenario from the spec:
 * if thread A pushes A and finishes returning before thread B pushes B,
 * then if thread C pops from the queue and finishes before thread D attempts to pop,
 * then thread C must pop A and thread D will have popped B.
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
        pthread_t tA, tB, tC, tD;
        void *rc;

        // Launch threads A, push A and finish returning
        pthread_create(&tA, NULL, threadA, (void *) q);
        pthread_join(tA, &rc);
        if (rc != NULL)
            return 1;
        // Launch thread C, pop a value (should be == 1)
        pthread_create(&tC, NULL, threadB, (void *) q);
        pthread_create(&tB, NULL, threadB, (void *) q);
        // C should return before attempting to launch thread D
        pthread_join(tC, &rc);
        if (rc != NULL)
            return 1;
        // Launch thead D, pop a value (should be == 2 upon thread B's completion)
        pthread_create(&tD, NULL, threadB, (void *) q);

        queue_delete(&q);
    }

    return 0;
}
