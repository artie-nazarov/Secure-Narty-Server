#include "code.h"
#include "defines.h"
#include "header.h"
#include "huffman.h"
#include "io.h"
#include "node.h"
#include "pq.h"
#include "stack.h"
#include "sys/stat.h"

#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// h = help, i = infile, o = outfile, v = verbose
#define OPTIONS "hvi:o:"

void print_instructions();
int tree_size;

int main(int argc, char *argv[]) {
    int opt = 0;
    char *infile;
    char *outfile;
    int fd_in = STDIN_FILENO; // file descriptor for infile
    int fd_out = STDOUT_FILENO; // file descriptor for outfile
    uint8_t buf[BLOCK]; // character buffer for reading and writing input/output
    int bytes_processed; // holds return value of read_bytes() and write_bytes()
    // arg flags
    int help = 0;
    int verbose = 0;
    int infile_given = 0;
    int outfile_given = 0;
    // decoding vars
    int buf_index;
    uint64_t decoded_symbols;
    uint8_t bit;
    extern uint64_t bytes_read; // tracks total bytes read from infile
    extern uint64_t bytes_written; // tracks total bytes written to outfile

    //user input loop
    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        //help menu
        case 'h': help = 1; break;
        case 'v': verbose = 1; break;
        case 'i':
            infile_given = 1;
            // open input file
            infile = optarg;
            fd_in = open(infile, O_RDONLY, 0);
            // error opening if open returns -1
            if (fd_in == -1) {
                fprintf(stderr, "failed to open input file\n");
                return 1;
            }
            break;
        case 'o':
            outfile_given = 1;
            // open output file
            outfile = optarg;
            fd_out = open(outfile, O_WRONLY, 0);
            // error opening if open returns -1
            if (fd_out == -1) {
                fprintf(stderr, "failed to open input file\n");
                return 1;
            }
            break;
        default: print_instructions(); break;
        }
    }
    if (help) {
        print_instructions();
        return 1;
    }

    bytes_read = 0;
    bytes_written = 0;

    // getting file permissions/stats
    struct stat statbuf;
    fstat(fd_in, &statbuf);

    // START DECODING PROCESS

    // READ HEADER
    Header h;
    read_bytes(fd_in, (uint8_t *) &h, sizeof(Header));
    if (h.magic != MAGIC) {
        dprintf(fd_out, "Error: unable to read header.\n");
        return 1;
    }

    // set file permissions
    fchmod(fd_out, h.permissions);

    // RECREATE TREE
    // initialize tree_dump array
    uint8_t tree_dump[h.tree_size];

    //READ INPUT AND POPULATE TREE DUMP
    bytes_processed = read_bytes(fd_in, buf, h.tree_size);
    if (bytes_processed == h.tree_size) {
        // for each character in buffer
        for (int i = 0; i < bytes_processed; i++) {
            tree_dump[i] = buf[i]; // add char to tree_dump array
        }
    } else {
        printf("error: huffman tree not read properly\n");
        return 1;
    }

    // REBUILD TREE
    Node *root = rebuild_tree(h.tree_size, tree_dump);

    // TRANSLATE CODES TO SYMBOLS
    Node *cur = root;
    buf_index = 0;
    decoded_symbols = 0;
    // use huffman tree to reconstruct origional messsage
    // itterate through each bit in input and traverse the tree until leaf is met
    while (read_bit(fd_in, &bit) && decoded_symbols <= h.file_size) {
        // if bit = 0, go left
        if (bit == 0) {
            cur = cur->left;
        }
        // else bit = 1, go right
        else {
            cur = cur->right;
        }
        // if cur is a leaf node
        if (cur->left == NULL && cur->right == NULL) {
            // add current symbol to output buffer
            buf[buf_index] = cur->symbol;
            buf_index += 1;
            decoded_symbols += 1;
            cur = root;
            // write out buffer if full
            if (buf_index == BLOCK) {
                write_bytes(fd_out, buf, BLOCK);
                buf_index = 0;
            }
        }
    }

    // write whatever remains on buffer
    if (buf_index > 0) {
        write_bytes(fd_out, buf, buf_index);
    }

    // print decoding stats if verbose is chosen
    if (verbose == 1) {
        float space_saved = 100 * (1 - ((float) bytes_read / (float) h.file_size));
        fprintf(stdout, "\n");
        fprintf(stdout, "Compressed file size: %" PRIu64 " bytes\n", bytes_read);
        fprintf(stdout, "Decompressed file size: %" PRIu64 " bytes\n", h.file_size);
        fprintf(stdout, "Space saving: %.2f%%\n", space_saved);
    }

    // delete tree
    delete_tree(&root);

    // close any opened files
    if (infile_given) {
        close(fd_in);
    }
    if (outfile_given) {
        close(fd_out);
    }
}

void print_instructions() {
    // Instructions
    printf("SYNOPSIS\n");
    printf("  A Huffman decoder.\n");
    printf("  Decompresses a file using the Huffman coding algorithm.\n\n");

    printf("USAGE\n");
    printf("  ./decode [-h] [-v] [-i infile] [-o outfile]\n\n");

    printf("OPTIONS\n");
    printf("  -h             Program usage and help.");
    printf("  -v             Print decompression statistics.");
    printf("  -i infile      Input file to decompress.\n");
    printf("  -o outfile     Output of decompressed data.\n");
}
