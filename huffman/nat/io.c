#include "code.h"
#include "defines.h"

#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

static uint8_t bit_buf[BLOCK]; // buffer for code bits
static int bit_index = 0; // index for bit_buf
// read_bit vars:
static int bread = 0; // bytes read from buffer in read_bit
static int last_bit = -1; // index of last bit in read_bit
uint64_t bytes_written;
uint64_t bytes_read;

// reads file until nbytes have been read or until there are no more bytes left to read
// returns number of bytes read
int read_bytes(int infile, uint8_t *buf, int nbytes) {
    int total = 0; // total num of bytes read
    extern uint64_t bytes_read;
    int n = read(infile, buf, nbytes); // bytes read by each call to read
    total += n;
    bytes_read += n;
    while (n > 0 && total < nbytes) {
        n = read(infile, buf, nbytes - total);
        total += n;
        bytes_written += n;
    }
    return total;
}

// write to file from buffer. Loops uitil nbytes were written and returns # bytes written
int write_bytes(int outfile, uint8_t *buf, int nbytes) {
    extern uint64_t bytes_written;
    int total = 0;
    int n = write(outfile, buf, nbytes);
    bytes_written += n;
    total += n;
    while (n > 0 && total < nbytes) {
        n = write(outfile, buf, nbytes - total);
        bytes_written += n;
        total += n;
    }
    return total;
}

// used to read in bits in decode
bool read_bit(int infile, uint8_t *bit) {
    // uses static bit buffer and vars bread, bit_index, and last_bit
    // if buffer's empty
    if (bit_index == 0) {
        bread = read_bytes(infile, bit_buf, BLOCK);
        if (bread < BLOCK) {
            last_bit = 8 * bread + 1;
        }
    }
    // pass back the bit
    *bit = (bit_buf[bit_index / 8] >> (bit_index % 8)) & 0x1;
    bit_index += 1; // increment bit_index
    if (bit_index == BLOCK * 8) {
        bit_index = 0;
    }
    // return T/F if bit_index has not reached end of buffer
    if (bit_index == last_bit) {
        return false;
    } else {
        return true;
    }
}

// buffer each bit of the code
// if buffer is full, write buffer out
void write_code(int outfile, Code *c) {
    for (uint32_t i = 0; i < code_size(c); i++) {
        // for every bit in c, if bit is 1, set bit in buffer, else clear it
        if (code_get_bit(c, i)) {
            bit_buf[bit_index / 8] |= (0x1 << (bit_index % 8));
        } else {
            bit_buf[bit_index / 8] &= ~(0x1 << (bit_index % 8));
        }
        bit_index += 1; // itterate buffer index
        // if buffer is full
        if (bit_index == 8 * BLOCK) {
            write_bytes(outfile, bit_buf, BLOCK); // write buffer to outfile
            bit_index = 0; // reset buffer index
        }
    }
}

// take num of bytes that contain bits in bit_buffer and write them to output
void flush_codes(int outfile) {
    if (bit_index > 0) {
        // num of bytes left in buffer: buff_index / 8 (+1 if not divisible by 8)
        int nbytes = bit_index % 8 == 0 ? bit_index / 8 : bit_index / 8 + 1;
        write_bytes(outfile, bit_buf, nbytes);
    }
}
