#include "queue.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#define QUEUE_SIZE 1
#define DELAY      1

void *threadA(void *args) {
    queue_t *q = ((void **) args)[0];
    struct timeval *start = (struct timeval *) ((void **) args)[1];
    struct timeval *end = (struct timeval *) ((void **) args)[2];
    gettimeofday(start, NULL);
    if (!queue_push(q, (void *) (intptr_t) 1)) {
        return (void *) 1;
    }
    gettimeofday(end, NULL);

    return NULL;
}

void *threadB(void *args) {
    queue_t *q = ((void **) args)[0];
    struct timeval *start = (struct timeval *) ((void **) args)[1];
    struct timeval *end = (struct timeval *) ((void **) args)[2];
    void *rv;
    gettimeofday(start, NULL);
    if (!queue_pop(q, &rv)) {
        return (void *) 1;
    }
    gettimeofday(end, NULL);

    return NULL;
}

/** Test 5 - Test whether our queue "hangs" when full and a producer attempts to push, until a consumer frees up the queue
 * Step 1: Launch thread A attempting to push onto a FULL queue. Add some delay before launching thread B, to give A time to start hanging.
 * Step 2: Launch another thread B that will free up space on the queue
 * Step 3: End time of thread B should be smaller than end time of A (thread B must finish before A).
*/
int main(int argc, char **argv) {
    if (argc != 2) {
        return 2;
    }
    int iters = atoi(argv[1]);
    for (int i = 0; i < iters; i++) {
        queue_t *q = queue_new(QUEUE_SIZE);
        // Fill up the queue
        for (int i = 0; i < QUEUE_SIZE; i++) {
            if (!queue_push(q, (void *) (intptr_t) 1)) {
                return 1;
            }
        }
        if (q == NULL) {
            return 1;
        }
        pthread_t tA, tB;
        struct timeval a_start, a_end, b_start, b_end;

        void *args1[3] = { q, (void *) &a_start, (void *) &a_end };
        void *args2[3] = { q, (void *) &b_start, (void *) &b_end };

        // Launch thread A. This thread will try to push onto a full queue, and should hang
        pthread_create(&tA, NULL, threadA, args1);
        // Help ensure thread A starts
        sleep(DELAY);
        // Launch thread B. This thread will pop off a full queue, unlocking thread A
        pthread_create(&tB, NULL, threadB, args2);

        void *rc;
        pthread_join(tA, &rc);
        if (rc != NULL)
            return 1;
        pthread_join(tB, &rc);
        if (rc != NULL)
            return 1;

        // Check the timings
        // thread B should have started before thread A (or right at the same), and should have finished before thread A finishes
        if (a_start.tv_sec > b_start.tv_sec) {
            return 1;
        }
        if (a_end.tv_sec < b_end.tv_sec) {
            return 1;
        }

        queue_delete(&q);
    }

    return 0;
}
