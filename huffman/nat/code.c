#include "code.h"

#include <inttypes.h>
#include <stdio.h>

Code code_init(void) {
    Code c;
    c.top = 0;
    return c;
}

uint32_t code_size(Code *c) {
    return c->top;
}

bool code_empty(Code *c) {
    return c->top == 0;
}

bool code_full(Code *c) {
    return c->top == ALPHABET; // 256 = ALPHABET = max capacity
}

bool code_push_bit(Code *c, uint8_t bit) {
    if (code_full(c)) {
        return false;
    } else if (bit == 0) {
        // AND byte with 11..0..11 to clear correct bit
        c->bits[c->top / 8] &= ~(0x1 << (c->top % 8));
        c->top += 1; // itterate top
    } else {
        // OR byte with 00..1..00 to set correct bit
        c->bits[c->top / 8] |= (0x1 << (c->top % 8));
        c->top += 1;
    }
    return true;
}

bool code_pop_bit(Code *c, uint8_t *bit) {
    if (code_empty(c)) {
        return false;
    } else {
        c->top -= 1;
        // shift og byte by bit offset, then AND with 0x1 to return single bit
        *bit = (c->bits[c->top / 8] >> (c->top % 8) & 0x1);
        return true;
    }
}

uint8_t code_get_bit(Code *c, uint32_t i) {
    return (c->bits[i / 8] >> (i % 8) & 0x1); // shift og vector right by offset, then && with 0x1
}

void code_print(Code *c) {
    for (uint32_t i = 0; i < c->top; i++) {
        printf("%" PRIu8, code_get_bit(c, i));
    }
}
