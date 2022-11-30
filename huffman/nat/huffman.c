#include "huffman.h"

#include "pq.h"
#include "stack.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int tree_size;

Node *build_tree(uint64_t hist[static ALPHABET]) {
    extern int tree_size;
    // create priority queue
    PriorityQueue *pq = pq_create(ALPHABET * 2);
    for (int i = 0; i < ALPHABET; i++) {
        // if frequency > 0, add node to pq
        if (hist[i] > 0) {
            // create node w character at index and cooresponding frequency
            Node *n = node_create(i, hist[i]);
            // enqueue node
            enqueue(pq, n);
            tree_size += 1; // increment exten var tree_size
        }
    }

    // construct Huffman tree from priority queue
    while (pq_size(pq) >= 2) {
        // create 2 placeholder nodes to dequeue into
        Node *l = node_create(' ', 0);
        Node *r = node_create(' ', 0);
        dequeue(pq, &l);
        dequeue(pq, &r);
        Node *parent = node_join(l, r);
        enqueue(pq, parent);
    }
    // dequeue root and return it
    Node *root = node_create(' ', 0);
    dequeue(pq, &root);
    // delete priority queue
    pq_delete(&pq);
    return root;
}

void build_codes(Node *root, Code table[static ALPHABET]) {
    Code c = code_init();
    // recursive call to post_order_traversal
    // should construct code as traversing through tree
    // and add code to table when leaf is reached
    post_order_traversal(root, c, table); // fills table with codes
}

Node *rebuild_tree(uint16_t nbytes, uint8_t tree[static nbytes]) {
    Stack *s = stack_create(nbytes);
    for (int i = 0; i < nbytes; i++) {
        if (tree[i] == 'L') {
            Node *n = node_create(tree[i + 1], 0);
            stack_push(s, n); // push node of char onto the stack
            i += 1; // increment i to not process char
        } else if (tree[i] == 'I') {
            Node *right = node_create(' ', 0);
            Node *left = node_create(' ', 0);
            // pop 2 nodes from stack and join
            stack_pop(s, &right);
            stack_pop(s, &left);
            Node *parent = node_join(left, right);
            stack_push(s, parent);
        }
    }
    Node *root;
    stack_pop(s, &root);
    stack_delete(&s);
    return root;
}

void delete_tree(Node **root) {
    if (*root) {
        if (&((*root)->left) != NULL) {
            delete_tree((&(*root)->left));
        }
        if ((&(*root)->right) != NULL) {
            delete_tree((&(*root)->right));
        }
        node_delete(root);
    }
}

void post_order_traversal(Node *root, Code c, Code table[static ALPHABET]) {
    if (root->left != NULL) {
        uint8_t pop_bit;
        code_push_bit(&c, 0); // add 0 to code for left traversal
        post_order_traversal(root->left, c, table); // recursive call to traversal on left branch
        code_pop_bit(&c, &pop_bit); // pop bit after recursing left
    }
    if (root->right != NULL) {
        uint8_t pop_bit;
        code_push_bit(&c, 1); // add 1 to code for right traversal
        post_order_traversal(root->right, c, table); // recursive traversal on right branch
        code_pop_bit(&c, &pop_bit); // pop bit after recursing left
    }
    // if node is a leaf
    table[root->symbol] = c; // append code to cooresponding character spot in table
}
