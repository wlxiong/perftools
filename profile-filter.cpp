#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <map>
#include "addr2line.h"
using namespace std;

#define PSIZE 8
#define INT_TYPE unsigned long long
#define FUNC_MAX (PATH_MAX)

int main(int argc, char* argv[]) {

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <program> <profile file> <symbols...>\n", argv[0]);
        exit(1);
    }

    char *program_name = argv[1];
    const char *section_name = NULL;
    char *target = NULL;
    if (0 != libtrace_init(program_name, section_name, target)) {
        fprintf(stderr, "Cannot initialize libtrace");
        exit(5);
    }
    char  func[FUNC_MAX+1] = {0};
    char  file[PATH_MAX+1] = {0}; 

    int i, num_sym = argc - 3;
    char **syms = &argv[3];
    for (i = 0; i < num_sym; i++) {
        fprintf(stderr, "Remove stack traces including symbol: %s\n", syms[i]);
    }

    FILE* profile = fopen(argv[2], "r");
    char header[PSIZE*2];
    if (fread(header, PSIZE*2, 1, profile) < 1) {
        fprintf(stderr, "Cannot read the first two headers");
        exit(2);
    }
    fwrite(header, PSIZE*2, 1, stdout);

    INT_TYPE header_slot = *(INT_TYPE*) &header[PSIZE];
    char* more_header = (char*) malloc(PSIZE*header_slot);
    if(fread(more_header, PSIZE*header_slot, 1, profile) < 1) {
        fprintf(stderr, "Cannot read more header slots");
        exit(3);
    }
    fwrite(more_header, PSIZE*header_slot, 1, stdout);

    INT_TYPE sampling_period = *(INT_TYPE*) &more_header[PSIZE];
    fprintf(stderr, "Sampling period, in microseconds: %llu\n", sampling_period);
    char record_header[PSIZE*2];
    int num_matched = 0;
    map<INT_TYPE, char*> addr2func;
    map<INT_TYPE, char*>::iterator it;
    while(1) {
        if (fread(record_header, PSIZE*2, 1, profile) < 1) {
            fprintf(stderr, "Cannot read profile record header");
            exit(4);
        }

        INT_TYPE sample_count = *(INT_TYPE*) &record_header[0];
        INT_TYPE num_call = *(INT_TYPE*) &record_header[PSIZE];
        int j, matched = 0;
        INT_TYPE pc;
        char* call_pc = (char*) malloc(PSIZE*num_call);
        fread(call_pc, PSIZE, num_call, profile);
        pc = *(INT_TYPE*) &call_pc[0];
        if (sample_count == 0 && num_call == 1 && pc == 0) {
            fwrite(record_header, PSIZE*2, 1, stdout);
            fwrite(call_pc, PSIZE, num_call, stdout);
            break;
        }

        for (j = 0; j < num_call; j++) {
            pc = *(INT_TYPE*) &call_pc[j*PSIZE];
            // fprintf(stderr, "%llx\n", pc);
            it = addr2func.find(pc);
            if (it == addr2func.end()) {
                libtrace_resolve((void *)pc, func, FUNC_MAX, file, PATH_MAX);
                addr2func.insert(make_pair(pc, strdup(func)));
            } else {
                func = it->second;
            }
            for (i = 0; i < num_sym; i++) {
                if (strcmp(func, syms[i]) == 0) {
                    matched = 1;
                    num_matched++;
                    goto has_matched;
                }
            }
        }
        has_matched:
        if (matched == 0) {
            fwrite(record_header, PSIZE*2, 1, stdout);
            fwrite(call_pc, PSIZE, num_call, stdout);
        }
    }
    fprintf(stderr, "Total matched stack traces: %d\n", num_matched);

    // output text part
    int c;
    while ((c = fgetc(profile)) != EOF) {
        fputc(c, stdout);
    }

    libtrace_close();
    fclose(profile);

    return 0;
}
