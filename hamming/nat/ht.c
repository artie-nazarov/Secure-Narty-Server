#include "ht.h"

#include "speck.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//uint64_t seeks; // Number of seeks performed.
//uint64_t links; // Number of links traversed.

struct HashTable {
    uint64_t salt[2];
    uint32_t size;
    bool mtf;
    LinkedList **lists;
};

HashTable *ht_create(uint32_t size, bool mtf) {
    HashTable *ht = (HashTable *) malloc(sizeof(HashTable));
    if (ht) {
        ht->salt[0] = 0x9846e4f157fe8840;
        ht->salt[1] = 0xc5f318d7e055afb8;
        ht->size = size;
        ht->mtf = mtf;
        ht->lists = (LinkedList **) calloc(size, sizeof(LinkedList *));
        if (!ht->lists) {
            free(ht);
            ht = NULL;
        }
    }
    return ht;
}

void ht_delete(HashTable **ht) {
    if (*ht && (*ht)->lists) {
        for (uint32_t i = 0; i < ht_size(*ht); i++) {
            // if list at index is not empty
            if ((*ht)->lists[i] != NULL) {
                ll_delete(&((*ht)->lists[i]));
            }
        }
        free((*ht)->lists);
        (*ht)->lists = NULL;
        free(*ht);
        *ht = NULL;
    }
}

uint32_t ht_size(HashTable *ht) {
    return ht->size;
}

// sees if word is in hash table at index of hash
Node *ht_lookup(HashTable *ht, char *oldspeak) {
    uint32_t index = hash(ht->salt, oldspeak) % ht_size(ht);
    // if no list at that hash table index, return null
    if (ht->lists[index] == NULL) {
        return NULL;
    }
    // else search linked list for word
    else {
        return ll_lookup(ht->lists[index], oldspeak);
    }
}

void ht_insert(HashTable *ht, char *oldspeak, char *newspeak) {
    // only insert if oldspeak not yet in ht
    //SEG FAULT on ll lookup
    uint32_t index = hash(ht->salt, oldspeak) % ht->size;
    if (ht->lists[index] == NULL) {
        ht->lists[index] = ll_create(ht->mtf);
    }
    //Seg Fault caused by line below: ll_insert
    ll_insert(ht->lists[index], oldspeak, newspeak);
}

uint32_t ht_count(HashTable *ht) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < ht->size; i++) {
        if (ht->lists[i] != NULL) {
            count++;
        }
    }
    return count;
}

void ht_print(HashTable *ht) {
    for (int i = 1; i <= (int) ht_size(ht); i++) {
        printf("hash[%i]: ", i);
        // if list at index is not empty
        if (ht->lists[i] != NULL) {
            ll_print(ht->lists[i]);
        }
        printf("\n");
    }
}
