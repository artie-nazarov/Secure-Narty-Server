#include "node.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
//STATUS: tested, all working besides problems with node_delete

Node *node_create(uint8_t symbol, uint64_t frequency) {
    Node *n = (Node *) malloc(sizeof(Node));
    n->symbol = symbol;
    n->frequency = frequency;
    return n;
}

void node_delete(Node **n) {
    if (*n) {
        free(*n);
        *n = NULL;
    }
    return;
}

// creates a new node with left and right as children
Node *node_join(Node *left, Node *right) {
    uint64_t parent_freq = left->frequency + right->frequency;
    Node *parent = node_create('$', parent_freq);
    parent->left = left;
    parent->right = right;
    return parent;
}

// print node's char, frequency, and all children
void node_print(Node *n) {
    // print in format (symbol, frequency)
    printf(" (%i, ", n->symbol);
    printf(" %" PRIu64 ") ", n->frequency);
}
