#include "queue.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ITEMS      100
#define QUEUE_SIZE 25

// Pops positive numbers onto the queue
void *threadC(void *arg) {
    queue_t *q = arg;

    for (int i = 0; i < ITEMS; i++) {
        if (!queue_push(q, (void *) (intptr_t) i)) {
            break;
        }
    }

    return NULL;
}

// Pops items off of the queue, checks element order
void *threadConsume(void *arg) {
    queue_t *q = arg;

    // checker variables
    int max = -ITEMS;
    void *rv;
    int res;
    for (int i = 0; i < ITEMS / 2; i++) {
        if (!queue_pop(q, &rv)) {
            return (void *) 1;
        }
        res = (int) rv;
        if (res > max) {
            max = res;
        } else {
            return (void *) 1;
        }
    }

    return NULL;
}

/** Test 7 - Thread Order (Multiple Consumer threads)
 * If a thread T1 enqueues item a and then enqueues b, any thread T2 that dequeues both a and b must dequeue a first.
 * Thise test will first complete push operations by thread A, and only then invoke pop operations by thread B
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
        pthread_t tA, tB, tC;

        // Launch producer thread tC
        pthread_create(&tC, NULL, threadC, (void *) q);
        // Launch Consumer threads tA and tB
        pthread_create(&tA, NULL, threadConsume, (void *) q);
        pthread_create(&tB, NULL, threadConsume, (void *) q);

        void *rc;
        pthread_join(tA, &rc);
        if (rc != NULL) {
            return 1;
        }
        pthread_join(tB, &rc);
        if (rc != NULL) {
            return 1;
        }
        pthread_join(tC, &rc);
        if (rc != NULL) {
            return 1;
        }

        queue_delete(&q);
    }
    return 0;
}
