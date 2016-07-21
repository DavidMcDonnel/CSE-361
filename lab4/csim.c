/*
 * David McDonnel 10/23/2015
 * WUID: dmcdonnel
 * CSE 361
 */
#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <errno.h>


int verbose = 0; 
int s = 0; 
int b = 0; 
int E = 0; 
char* trace = NULL;
unsigned hits = 0;
unsigned misses = 0;
unsigned evictions = 0;


typedef unsigned long long int memAdr;


//Line has a valid bit, tag, and lru
struct Line {
    char valid;
    memAdr tag; 
    unsigned lru;
};


struct Line *** cache;


// Create cache instance cache[set[Line]]
void make_cache() {
    //cache of sets
    cache = (struct Line ***) malloc(pow(2, s) * sizeof(struct Line **));
    //set of lines
    for (int set = 0; set < pow(2, s); ++set) {
        cache[set] = (struct Line **) malloc(E * sizeof(struct Line *));
	//Book keep each Line and its valid bit, tag, and lru
        for (int ln = 0; ln < E; ++ln) {
            cache[set][ln] = 
                (struct Line *) malloc(sizeof(struct Line));
            (cache[set][ln])->valid = 0;
            (cache[set][ln])->tag = 0;
            (cache[set][ln])->lru = 0;
        }
    }
}


// Run the trace file
void run_trace(char* trace_fn) {
    char buf[1000];
    memAdr addr=0;
    unsigned int len=0;
    FILE* trace_fp = fopen(trace_fn, "r");

    if(!trace_fp) {
        fprintf(stderr, "%s: %s\n", trace_fn, strerror(errno));
        exit(1);
    }

    memAdr block_bits = pow(2,b) - 1;
    memAdr set_bits = pow(2,s) - 1;
    set_bits <<= (b);
    memAdr tagMask = ~(block_bits|set_bits);

    while( fgets(buf, 1000, trace_fp) != NULL) {
        if(buf[1]=='S' || buf[1]=='L' || buf[1]=='M') {
            sscanf(buf+3, "%llx,%u", &addr, &len);
            if(verbose){
                printf("%c %llx,%u\n", buf[1], addr, len);
	    }

            memAdr set = (set_bits & addr) >> b;
            memAdr tag = (tagMask & addr) >> (s + b);
            int i = 1;
            switch(buf[1]) {
            case 'M':
                i = 2;
            case 'S':
            case 'L': {
                for (int iter = 0; iter < i; ++iter) {
                    char full_set = 1;
                    int empty_line = 0; 
                    char hit_ = 0;
                    for (int ln = 0; ln < E; ++ln) {
                        if ((cache[set][ln])->valid) {
                            if ((cache[set][ln])->tag == tag) {
                                ++hits;
                                for (int i = 0; i < E; ++i)
                                    if ((cache[set][i])->valid)
                                        ++((cache[set][i])->lru);
                                (cache[set][ln])->lru = 0;
                                hit_ = 1;
                                break;
                            }
                        }
                        else {
                            empty_line = ln;
                            full_set = 0;
                        }
                    }
                    if (!hit_) {
                        ++misses;
                        if (!full_set) {
                            (cache[set][empty_line])->valid = 1;
                            (cache[set][empty_line])->tag = tag;
                            for (int i = 0; i < E; ++i)
                                if ((cache[set][i])->valid)
                                    ++((cache[set][i])->lru);
                            (cache[set][empty_line])->lru = 0;
                        }
                        else {
                            ++evictions;
                            int max_lru_offset = 0;
                            int max_lru = 0;
                            for (int i = 0; i < E; ++i) {
                                if ((cache[set][i])->lru >
                                     max_lru) {
                                    max_lru = (cache[set][i])->lru;
                                    max_lru_offset = i;
                                }
                            }
                            (cache[set][max_lru_offset])->tag = tag;
                            for (int i = 0; i < E; ++i) {
                                ++((cache[set][i])->lru);
                            }
                            (cache[set][max_lru_offset])->lru = 0;
                        }
                    } 
                }
                break;
            }
            default:
                break;
            }
        }
    }
    fclose(trace_fp);
}


// Empty the cache
void clear_cache() {
    for (int set = 0; set < pow(2, s); ++set) {     
        for (int ln = 0; ln < E; ++ln) {
            free(cache[set][ln]);
        }
        free(cache[set]);
    }
    free(cache);
}


void printUsage(char* argv[]) {
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}


int main(int argc, char* argv[]) {
    char c;
    while((c=getopt(argc,argv,"s:E:b:t:vh")) != -1) {
        switch(c) {
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            trace = optarg;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
            printUsage(argv);
            exit(0);
        default:
            printUsage(argv);
            exit(1);
        }
    }
    if (s == 0 || E == 0 || b == 0 || trace == NULL) {
        printf("%s: Missing argument\n", argv[0]);
        printUsage(argv);
        exit(1);
    }
    make_cache();
    run_trace(trace);
    clear_cache();
    printSummary(hits, misses, evictions);
    return 0;
}
