#include "queue.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ITEMS      100
#define QUEUE_SIZE 25

// Pushes negative numbers onto the queue
void *threadA(void *arg) {
    queue_t *q = arg;

    for (int i = -1 * (ITEMS / 2); i < 0; i++) {
        if (!queue_push(q, (void *) (intptr_t) i)) {
            break;
        }
    }

    return NULL;
}

// Pops positive numbers onto the queue
void *threadB(void *arg) {
    queue_t *q = arg;

    for (int i = 0; i < (ITEMS / 2); i++) {
        if (!queue_push(q, (void *) (intptr_t) i)) {
            break;
        }
    }

    return NULL;
}

// Pops all items off of the queue
void *threadConsume(void *arg) {
    queue_t *q = arg;

    // checker variables
    int neg_max = -ITEMS, pos_max = -ITEMS;
    void *rv;
    int res;
    for (int i = 0; i < ITEMS; i++) {
        if (!queue_pop(q, &rv)) {
            return (void *) 1;
        }
        res = (int) rv;
        // Check whether num is positive or negative -> pushed by thread A or B respectively
        if (res < 0) {
            if (res > neg_max) {
                neg_max = res;
            } else {
                return (void *) 1;
            }
        } else {
            if (res > pos_max) {
                pos_max = res;
            } else {
                return (void *) 1;
            }
        }
    }

    return NULL;
}

/** Test 8 - Thread Order (Multiple Producer and Consumer threads)
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
        pthread_t tA, tB, tC1, tC2, tC3, tC4;

        // Launch producer threads tA and tB
        pthread_create(&tA, NULL, threadA, (void *) q);
        pthread_create(&tB, NULL, threadB, (void *) q);
        // Launch consumer threads
        pthread_create(&tC1, NULL, threadConsume, (void *) q);
        pthread_create(&tC2, NULL, threadConsume, (void *) q);
        pthread_create(&tC3, NULL, threadConsume, (void *) q);
        pthread_create(&tC4, NULL, threadConsume, (void *) q);

        void *rc;
        pthread_join(tC1, &rc);
        if (rc != NULL) {
            return 1;
        }
        pthread_join(tC2, &rc);
        if (rc != NULL) {
            return 1;
        }
        pthread_join(tC3, &rc);
        if (rc != NULL) {
            return 1;
        }
        pthread_join(tC4, &rc);
        if (rc != NULL) {
            return 1;
        }
        pthread_join(tA, &rc);
        if (rc != NULL) {
            return 1;
        }
        pthread_join(tB, &rc);
        if (rc != NULL) {
            return 1;
        }

        queue_delete(&q);
    }
    return 0;
}
