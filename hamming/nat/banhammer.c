#include "bf.h"
#include "bv.h"
#include "ht.h"
#include "ll.h"
#include "messages.h"
#include "node.h"
#include "parser.h"
#include "speck.h"

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define OPTIONS "hmst:f:"
#define BLOCK   4096
#define WORD    "([a-zA-Z0-9])+(('|-)([a-zA-Z0-9])+)*"

void print_instructions();

int main(int argc, char *argv[]) {
    int opt = 0;
    bool mtf = 0;
    int stats = 0;
    int help = 0;
    uint32_t hash_size = 10000;
    uint32_t bloom_size = pow(2, 20); // 2 ^ 20
    char *new_word;
    FILE *infile;
    regex_t regex;
    LinkedList *bad = ll_create(false); // for recording badspeak words
    LinkedList *old = ll_create(false); // for recording oldspeak words
    int thoughtcrime = 0; // for using badspeak
    int rightspeak = 0; // for using oldspeak
    extern uint64_t seeks;
    extern uint64_t links;
    //user input loop
    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        //help menu
        case 'h': help = 1; break;
        // verbpse: print compression stats
        case 'm': mtf = true; break;
        case 's': stats = 1; break;
        // given size of ht
        case 't': hash_size = (uint32_t) atoi(optarg); break;
        // given size of bf
        case 'f': bloom_size = (uint32_t) atoi(optarg); break;
        default: print_instructions(); break;
        }
    }
    if (help) {
        print_instructions();
        return 1;
    }
    seeks = 0;
    links = 0;
    // Initialize hash table
    HashTable *ht = ht_create(hash_size, mtf);
    // Initialize bloom filter
    BloomFilter *bf = bf_create(bloom_size);

    // Open badspeak.txt
    infile = fopen("badspeak.txt", "r");
    if (infile == NULL) {
        fprintf(stderr, "failed to open badspeak input file");
        return 1;
    }

    struct stat sb;
    if (stat("badspeak.txt", &sb) == -1) {
        perror("stat err");
        exit(EXIT_FAILURE);
    }

    // allocate badspeak to have enough room for infile
    char *badspeak = malloc(sb.st_size);

    // read in badspeak
    while ((fscanf(infile, "%s\n", badspeak)) != -1) {
        bf_insert(bf, badspeak);
        // ht_insert is causing seg fault vv
        ht_insert(ht, badspeak, NULL);
    }

    // Open newspeak
    infile = fopen("newspeak.txt", "r");
    if (infile == NULL) {
        fprintf(stderr, "failed to open newspeak input file");
        return 1;
    }

    struct stat sn;
    if (stat("newspeak.txt", &sn) == -1) {
        perror("stat err");
        exit(EXIT_FAILURE);
    }

    // allocate badspeak to have enough room for infile
    char *newspeak = malloc(sn.st_size);

    // read in newspeak
    while ((fscanf(infile, "%s %s\n", badspeak, newspeak)) != EOF) {
        bf_insert(bf, badspeak);
        ht_insert(ht, badspeak, newspeak);
    }

    // set input stream to stdin
    infile = stdin;

    // read user input to filter
    if (regcomp(&regex, WORD, REG_EXTENDED)) {
        perror("regcomp");
        exit(1);
    }
    while ((new_word = next_word(infile, &regex)) != NULL) {
        // convert word to lower
        uint32_t j = 0;
        char *word = malloc(strlen(new_word) + 1);
        // for each char in word, convert to lower and add to new str
        while (j < strlen(new_word)) {
            word[j] = tolower(new_word[j]);
            j++;
        }

        // bloom filter check
        if (bf_probe(bf, word) == true) {
            // hash table check
            // n is the node containing that badword in hash table, or NULL if not a bad word
            Node *n = ht_lookup(ht, word);
            if (n != NULL) {
                // use of badspeak
                if (n->newspeak == NULL) {
                    // record badspeak behavior
                    ll_insert(bad, word, NULL);
                    thoughtcrime = 1;
                } else {
                    // record oldspeak behavior
                    ll_insert(old, word, n->newspeak);
                    rightspeak = 1;
                }
            }
        }
    }

    // stats
    if (stats) {
        float avg_seek_len = (float) links / (float) seeks;
        float ht_load = 100 * ((float) ht_count(ht) / (float) hash_size);
        float bf_load = 100 * ((float) bf_count(bf) / (float) bloom_size);
        printf("Links: %" PRIu64 "\n", links); //TODO: remove
        printf("Seeks: %" PRIu64 " \n", seeks);
        printf("Average seek length: %f\n", avg_seek_len);
        printf("Hash table load: %f%%\n", ht_load);
        printf("Bloom filter load: %f%%\n", bf_load);
    } else {
        if (thoughtcrime && rightspeak) {
            printf("%s", mixspeak_message);
            ll_print(bad);
            ll_print(old);
        } else if (thoughtcrime) {
            printf("%s", badspeak_message);
            ll_print(bad);
        } else if (rightspeak) {
            printf("%s", goodspeak_message);
            ll_print(old);
        } else {
            printf("\n");
        }
    }

    // clear memory
    fclose(infile);
    clear_words();
    regfree(&regex);
    ht_delete(&ht);
    bf_delete(&bf);
    ll_delete(&old);
    ll_delete(&bad);
}

void print_instructions() {
    // Instructions
    printf("SYNOPSIS\n");
    printf("  A word filtering program for the GPRSC.\n");
    printf("  Filters out and reports bad words parsed from stdin.\n\n");

    printf("USAGE\n");
    printf("  ./banhammer [-hsm] [-t size] [-f size]\n\n");

    printf("OPTIONS\n");
    printf("  -h             Program usage and help.");
    printf("  -s           Print program statistics.");
    printf("  -m           Enable move-to-front rule.\n");
    printf("  -t size      Specify hash table size (default: 10000).\n");
    printf("  -f size      Specify Bloom filter size (default: 2^20).\n");
}
