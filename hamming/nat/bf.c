#include "bf.h"

#include "bv.h"
#include "speck.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct BloomFilter {
    uint64_t primary[2]; // salt #1
    uint64_t secondary[2]; // salt #2
    uint64_t tertiary[2]; // salt #3
    BitVector *filter;
};

BloomFilter *bf_create(uint32_t size) {
    BloomFilter *bf = (BloomFilter *) malloc(sizeof(BloomFilter));
    if (bf) {
        // salt 1
        bf->primary[0] = 0x5adf08ae86d36f21;
        bf->primary[1] = 0xa267bbd3116f3957;
        // salt 2
        bf->secondary[0] = 0x419d292ea2ffd49e;
        bf->secondary[1] = 0x09601433057d5786;
        // salt 3
        bf->tertiary[0] = 0x50d8bb08de3818df;
        bf->tertiary[1] = 0x4deaae187c16ae1d;
        bf->filter = bv_create(size);
        if (!bf->filter) {
            free(bf);
            bf = NULL;
        }
    }
    return bf;
}

void bf_delete(BloomFilter **bf) {
    // free and set to NULL
    // also use bv_delete()
    bv_delete(&((*bf)->filter));
    free(*bf);
    *bf = NULL;
}

uint32_t bf_size(BloomFilter *bf) {
    return bv_length(bf->filter);
}

void bf_insert(BloomFilter *bf, char *oldspeak) {
    uint32_t index = hash(bf->primary, oldspeak) % bf_size(bf);
    bv_set_bit(bf->filter, index);
    index = hash(bf->secondary, oldspeak) % bf_size(bf);
    bv_set_bit(bf->filter, index);
    index = hash(bf->tertiary, oldspeak) % bf_size(bf);
    bv_set_bit(bf->filter, index);
}

bool bf_probe(BloomFilter *bf, char *oldspeak) {
    uint32_t index = hash(bf->primary, oldspeak) % bf_size(bf);
    uint8_t first = bv_get_bit(bf->filter, index);
    index = hash(bf->secondary, oldspeak) % bf_size(bf);
    uint8_t second = bv_get_bit(bf->filter, index);
    index = hash(bf->tertiary, oldspeak) % bf_size(bf);
    uint8_t third = bv_get_bit(bf->filter, index);

    return (first && second && third);
}

uint32_t bf_count(BloomFilter *bf) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < bf_size(bf); i++) {
        count += bv_get_bit(bf->filter, i);
    }
    return count;
}

void bf_print(BloomFilter *bf) {
    bv_print(bf->filter);
}
