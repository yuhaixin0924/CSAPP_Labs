#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

typedef struct {
    int valid;
    unsigned long tag;
    unsigned long last_used;
} CacheLine;
int main(int argc, char *argv[])
{
    unsigned long timer=0;

    int hit=0;
    int miss=0;
    int eviction=0;

    unsigned long set_index;
    unsigned long tag;

    int s = 0;
    int E = 0;
    int b = 0;
    char *trace_file = NULL;
    int option;

    while ((option = getopt(argc, argv, "s:E:b:t:")) != -1) {
        switch (option) {
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
            trace_file = optarg;
            break;
        default:
            return 1;
        }
    }

    int set_count = 1 << s;
    int line_count = set_count * E;

    CacheLine *cache =
        calloc(line_count, sizeof(CacheLine));

    if (cache == NULL) {
        return 1;
    }

    FILE *file = fopen(trace_file, "r");

    if (file == NULL) {
        free(cache);
        return 1;
    }

    char operation;
    unsigned long address;
    int size;

    while (fscanf(file, " %c %lx,%d",
                  &operation, &address, &size) == 3) {
        // printf("%c %lx,%d\n", operation, address, size);
        set_index = (address >> b) & ((1UL << s) - 1);
        tag = address >> (s + b);
        switch (operation)
        {
        case 'L':
        case 'S':{
            int empty_line=-1;
            int temp_index=set_index*E;
            while(temp_index<(set_index+1)*E){
                if(cache[temp_index].valid==1&&cache[temp_index].tag==tag){
                    hit++;
                    timer++;
                    cache[temp_index].last_used = timer;
                    break;
                }
                else if(cache[temp_index].valid==0){
                    empty_line=temp_index;
                }
                temp_index++;
            }
            if(temp_index==(set_index+1)*E){
                miss++;
                if(empty_line!=-1){
                    timer++;
                    cache[empty_line].tag=tag;
                    cache[empty_line].last_used=timer;
                    cache[empty_line].valid=1;
                }
                else{
                    timer++;
                    eviction++;
                    unsigned long earliest=__LONG_LONG_MAX__;
                    int earliest_index;
                    for(int i=0;i<E;i++){
                       if(cache[i+set_index*E].last_used<earliest){
                            earliest=cache[i+set_index*E].last_used;
                            earliest_index=i+set_index*E;
                       }
                    }
                    cache[earliest_index].tag=tag;
                    cache[earliest_index].last_used=timer;
                    cache[earliest_index].valid=1;
                }
            }
            break;
        }
        case 'M':{
            for(int j=0;j<2;j++){
                int empty_line=-1;
                int temp_index=set_index*E;
                while(temp_index<(set_index+1)*E){
                    if(cache[temp_index].valid==1&&cache[temp_index].tag==tag){
                        hit++;
                        timer++;
                        cache[temp_index].last_used = timer;
                        break;
                    }
                    else if(cache[temp_index].valid==0){
                        empty_line=temp_index;
                    }
                    temp_index++;
                }
                if(temp_index==(set_index+1)*E){
                    miss++;
                    if(empty_line!=-1){
                        timer++;
                        cache[empty_line].tag=tag;
                        cache[empty_line].last_used=timer;
                        cache[empty_line].valid=1;
                    }
                    else{
                        timer++;
                        eviction++;
                        unsigned long earliest=__LONG_LONG_MAX__;
                        int earliest_index;
                        for(int i=0;i<E;i++){
                        if(cache[i+set_index*E].last_used<earliest){
                                earliest=cache[i+set_index*E].last_used;
                                earliest_index=i+set_index*E;
                        }
                        }
                        cache[earliest_index].tag=tag;
                        cache[earliest_index].last_used=timer;
                        cache[earliest_index].valid=1;
                        }
                    }
                }
            }
            break;
        default:
            break;
        }
    }

    fclose(file);
    free(cache);

    printSummary(hit, miss, eviction);
    return 0;
}