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
#include <sys/types.h>
#include <unistd.h>

// h = help, i = infile, o = outfile, v = verbose
#define OPTIONS "hvi:o:"

void write_tree(int outfile, Node *root);
void print_instructions();
int tree_size;
static uint8_t tree_buf[BLOCK];
static int tree_buf_index;

int main(int argc, char *argv[]) {
    int opt = 0;
    char *infile;
    char *outfile;
    int fd_in = STDIN_FILENO; // file descriptor for infile
    int fd_out = STDOUT_FILENO; // file descriptor for outfile
    uint8_t buf[BLOCK]; // character buffer for reading and writing input/output
    int bytes_processed; // holds return value of read_bytes() and write_bytes()
    int help = 0;
    int verbose = 0;
    int infile_given = 0;
    int outfile_given = 0;
    tree_buf_index = 0;
    extern int tree_size; // num of unique characters in tree
    extern uint64_t bytes_read; // tracks total bytes read from infile
    extern uint64_t bytes_written; // tracks total bytes written to outfile
    uint64_t hist[ALPHABET]; // histogram of character frequencies
    Code table[ALPHABET]; // table of cooresponding character codes

    //user input loop
    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        //help menu
        case 'h': help = 1; break;
        // verbpse: print compression stats
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
    //  Getting  and  setting  file  permissions
    struct stat statbuf;
    fstat(fd_in, &statbuf);
    fchmod(fd_out, statbuf.st_mode);

    // START ENCODING PROCESS

    //zero out histogram
    for (int i = 0; i < ALPHABET; i++) {
        // increment elements 0 and 255
        if (i == 0 || i == 255) {
            hist[i] = 1;
        }
        // otherwise initialize to 0
        else {
            hist[i] = 0;
        }
    }

    //READ INPUT AND POPULATE HIST
    while ((bytes_processed = read_bytes(fd_in, buf, BLOCK)) > 0) {
        // for each character in buffer
        for (int i = 0; i < bytes_processed; i++) {
            hist[buf[i]] += 1; // increment counter of hist at character read
        }
    }

    // build Huffman tree from histogram
    tree_size = 0;
    Node *root = build_tree(hist);

    // INITIATE CODE TABLE
    for (int i = 0; i < ALPHABET; i++) {
        table[i] = code_init();
    }
    // populate code table with char codes from tree
    build_codes(root, table);

    // CONSTRUCT HEADER
    Header header;
    header.magic = MAGIC;
    // header.permissions = fstat(fd_in, &statbuf);
    header.permissions = fstat(fd_in, &statbuf);
    header.tree_size = (3 * tree_size) - 1; // tree_size = # of unique chars in tree
    header.file_size = statbuf.st_size; // from fstat above

    // WRITE HEADER
    int n = write_bytes(fd_out, (uint8_t *) &header, sizeof(Header)); // writes header to outfile

    // WRITE TREE TO OUTFILE
    tree_buf_index = 0;
    write_tree(fd_out, root);
    if (tree_buf_index > 0) {
        n = write_bytes(fd_out, tree_buf, tree_buf_index);
        tree_buf_index = 0;
    }

    // SECOND PASS TO OUTPUT ENCRYPTED DATA
    lseek(fd_in, 0, SEEK_SET); // get back to beginning of file input

    //read from file one BLOCK at a time
    while ((bytes_processed = read_bytes(fd_in, buf, BLOCK)) > 0) {
        // for each character in buffer
        for (int i = 0; i < bytes_processed; i++) {
            // write out the code cooresponding to that char
            write_code(fd_out, &table[buf[i]]); // bytes_written incremented in write_code()
        }
    }
    // write remaining bits if anything left in buffer
    flush_codes(fd_out); // bytes written incremented in function

    // print encoding stats if verbose is chosen
    if (verbose == 1) {
        float space_saved = 100 * (1 - ((float) bytes_written / (float) header.file_size));
        fprintf(stdout, "\n");
        fprintf(stdout, "Uncompressed file size: %" PRIu64 " bytes\n", header.file_size);
        fprintf(stdout, "Compressed file size: %" PRIu64 " bytes\n", bytes_written);
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

    return 0;
}

void print_instructions() {
    // Instructions
    printf("SYNOPSIS\n");
    printf("  A Huffman encoder.\n");
    printf("  Compresses a file using the Huffman coding algorithm.\n\n");

    printf("USAGE\n");
    printf("  ./encode [-h] [-v] [-i infile] [-o outfile]\n\n");

    printf("OPTIONS\n");
    printf("  -h             Program usage and help.");
    printf("  -v             Print compression statistics.");
    printf("  -i infile      Input data to encode.\n");
    printf("  -o outfile     Output of encoded data.\n");
}

void write_tree(int outfile, Node *root) {
    // buffer full handling
    if (tree_buf_index == BLOCK - 1) {
        write_bytes(outfile, tree_buf, tree_buf_index);
        tree_buf_index = 0;
    }

    // if leaf node
    if (root->right == NULL && root->left == NULL) {
        // print out L for leaf
        tree_buf[tree_buf_index] = 'L';
        // print out symbol of node
        tree_buf[tree_buf_index + 1] = root->symbol;
        tree_buf_index += 2;
    }
    // if node is a parent, recurse down left then right then add an I
    else {
        if (root->left != NULL) {
            write_tree(outfile, root->left);
        }
        if (root->right != NULL) {
            write_tree(outfile, root->right);
        }
        tree_buf[tree_buf_index] = 'I';
        tree_buf_index += 1;
    }
}
