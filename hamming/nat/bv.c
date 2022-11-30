#include "bv.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct BitVector {
    uint32_t length;
    uint8_t *vector;
};

BitVector *bv_create(uint32_t length) {
    BitVector *v = (BitVector *) calloc(1, sizeof(BitVector)); // 1 refers to 1 byte
    if (!v) {
        return NULL;
    }
    v->length = length;
    // if not divisible by 8, needs 1 extra byte to hold length
    int val = length % 8 == 0 ? length / 8 : length / 8 + 1;
    v->vector
        = (uint8_t *) calloc(val, sizeof(uint8_t)); // length is in bits, need to convert to bytes
    if (!v->vector) {
        return NULL;
    }
    return v;
}

void bv_delete(BitVector **bv) {
    free((*bv)->vector);
    free(*bv);
    *bv = NULL;
}

uint32_t bv_length(BitVector *bv) {
    return bv->length;
}

void bv_set_bit(BitVector *bv, uint32_t i) {
    //set bit at i to 1 and or with og data
    bv->vector[i / 8] |= (0x1 << (i % 8)); // || og byte with 00..1..00
}

void bv_clr_bit(BitVector *bv, uint32_t i) {
    // set bit at i to 0
    bv->vector[i / 8] &= ~(0x1 << (i % 8)); // && og byte with 11..0..11
}

uint8_t bv_get_bit(BitVector *bv, uint32_t i) {
    return (
        bv->vector[i / 8] >> (i % 8) & 0x1); // shift og vector right by offset, then && with 0x1
}

void bv_print(BitVector *bv) {
    for (uint32_t i = 0; i < bv->length; i++) {
        printf("%3" PRIu8, bv_get_bit(bv, i));
    }
    printf("\n");
}
