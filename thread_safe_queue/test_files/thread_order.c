#include "queue.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define QUEUE_SIZE 10

void *threadA(void *args) {
    queue_t *q = ((void **) args)[0];
    int *ints = (int *) (((void **) args)[1]);

    for (int i = 0; i < QUEUE_SIZE; i++) {
        if (!queue_push(q, (void *) (intptr_t) ints[i])) {
            break;
        }
    }

    return NULL;
}

void *threadB(void *args) {
    queue_t *q = ((void **) args)[0];
    int *ints = (int *) (((void **) args)[1]);

    void *rv;
    for (int i = 0; i < QUEUE_SIZE; i++) {
        if (!queue_pop(q, &rv) || (int) rv != (int) ints[i]) {
            return (void *) 1;
        }
    }

    return NULL;
}

/** Test 1 - Thread Order
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
        pthread_t tA, tB;

        // Array of 10 intergers.
        int arr[QUEUE_SIZE] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        void *args[2] = { q, (void *) arr };

        // Push 10 elements to queue within thread A
        pthread_create(&tA, NULL, threadA, args);
        void *rc;
        pthread_join(tA, &rc);
        if (rc != NULL)
            return 1;

        // Pop 10 elements off the queue within thread B. Checks the correctness of element ordering
        pthread_create(&tB, NULL, threadB, args);
        pthread_join(tB, &rc);
        if (rc != NULL)
            return 1;

        queue_delete(&q);
    }
    return 0;
}
