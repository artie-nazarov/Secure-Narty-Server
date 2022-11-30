#include "ll.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint64_t seeks; // Number of seeks performed.
uint64_t links; // Number of links traversed.

struct LinkedList {
    uint32_t length;
    Node *head;
    Node *tail;
    bool mtf; // move to front flag
};

LinkedList *ll_create(bool mtf) {
    LinkedList *ll = (LinkedList *) malloc(sizeof(LinkedList));
    ll->head = node_create(NULL, NULL);
    ll->tail = node_create(NULL, NULL);
    ll->tail->prev = ll->head;
    ll->head->next = ll->tail;
    ll->mtf = mtf;
    ll->length = 0;
    return ll;
}

void ll_delete(LinkedList **ll) {
    while ((*ll)->head != NULL) {
        Node *temp = (*ll)->head->next;
        node_delete(&(*ll)->head);
        (*ll)->head = temp;
    }
    free(*ll);
    *ll = NULL;
}

uint32_t ll_length(LinkedList *ll) {
    return ll->length;
}

Node *ll_lookup(LinkedList *ll, char *oldspeak) {
    extern uint64_t seeks;
    extern uint64_t links;
    seeks += 1;
    // for each node in linked list
    for (Node *curr = ll->head->next; curr != ll->tail; curr = curr->next) {
        // only do strcmp if both strings are not NULL
        if (oldspeak != NULL && curr->oldspeak != NULL) {
            // if oldspeak of node matches searched oldspeak
            if (strcmp(curr->oldspeak, oldspeak) == 0) {
                // node was found, so if mtf enabled move queried node to front
                if (ll->mtf) {
                    curr->prev->next = curr->next;
                    curr->next->prev = curr->prev;
                    curr->next = ll->head->next;
                    curr->prev = ll->head;
                    ll->head->next = curr;
                    curr->next->prev = curr;
                }
                return curr;
            }
        }
        links += 1;
    }
    return NULL;
}

// insert node at head
void ll_insert(LinkedList *ll, char *oldspeak, char *newspeak) {
    // seach linked list to see if oldspeak has already been inserted
    if (ll_lookup(ll, oldspeak) == NULL) {
        // insert node
        Node *n = node_create(oldspeak, newspeak);
        n->prev = ll->head;
        n->next = ll->head->next;
        ll->head->next = n;
        n->next->prev = n;
    }
}

void ll_print(LinkedList *ll) {
    // for each node in list, print out node
    for (Node *curr = ll->head->next; curr != ll->tail; curr = curr->next) {
        node_print(curr);
        printf("\n");
    }
}
