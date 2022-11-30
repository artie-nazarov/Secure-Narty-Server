#include "pq.h"

#include "node.h"

#include <stdio.h>
#include <stdlib.h>

struct PriorityQueue {
    Node *nodes;
    uint32_t head;
    uint32_t tail;
    uint32_t size;
    uint32_t capacity;
};

PriorityQueue *pq_create(uint32_t capacity) {
    PriorityQueue *pq = (PriorityQueue *) malloc(sizeof(PriorityQueue));
    if (pq) {
        pq->head = 0;
        pq->tail = 0;
        pq->size = 0;
        pq->capacity = capacity; // set initial size of queue to given capacity
        pq->nodes = (Node *) calloc(capacity, sizeof(Node));
    }
    return pq;
}

void pq_delete(PriorityQueue **q) {
    if (*q && (*q)->nodes) {
        free((*q)->nodes);
        free(*q);
        *q = NULL;
    }
    return;
}

bool pq_empty(PriorityQueue *q) {
    return (q->size == 0);
}

bool pq_full(PriorityQueue *q) {
    return (q->capacity == q->head);
}

uint32_t pq_size(PriorityQueue *q) {
    return q->size;
}

bool enqueue(PriorityQueue *q, Node *n) {
    if (pq_full(q)) {
        return false;
    } else {
        // insert n to tail of queue
        q->nodes[q->tail] = *n; // add n to end of queue
        q->size += 1; // increment size
        // while at least 1 more element in front of tail and element n is smaller than element ahead of it
        int j = q->tail;
        while (((j - q->head) >= 1) && ((q->nodes[j]).frequency < (q->nodes[j - 1]).frequency)) {
            // swap n with item ahead of it in array if smaller
            q->nodes[j] = q->nodes[j - 1];
            q->nodes[j - 1] = *n;
            j--;
        }
        q->tail = (q->tail + 1) % q->capacity; // increment tail in circular queue
        return true;
    }
}

bool dequeue(PriorityQueue *q, Node **n) {
    if (q && !pq_empty(q)) {
        q->size -= 1;
        **n = q->nodes[q->head];
        q->head = (q->head + 1) % q->capacity;
        return true;
    }
    return false;
}

void pq_print(PriorityQueue *q) {
    printf("[ ");
    for (uint32_t i = q->head; i < q->tail; i++) {
        node_print(&(q->nodes[i]));
        if (i + 1 != q->tail) {
            printf(", ");
        }
    }
    printf("]\n");
}
