#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PSIZE 8
#define INT_TYPE unsigned long long

int main(int argc, char* argv[]) {

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <profile file> <address...>", argv[0]);
        exit(1);
    }
    int i, num_addr = argc - 2;
    INT_TYPE *addr = (INT_TYPE*) malloc(num_addr * sizeof(INT_TYPE));
    for (i = 0; i < num_addr; i++) {
        sscanf(argv[2 + i], "%llx", &addr[i]);
        fprintf(stderr, "Remove stack traces including address: %llx\n", addr[i]);
    }
    FILE* profile = fopen(argv[1], "r");
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
            for (i = 0; i < num_addr; i++) {
                if (pc == addr[i]) {
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

    fclose(profile);

    return 0;
}
