#include <iostream>
#include <string.h>
#include "CMS.hpp"
#include "genzipf.h"

int test_baseline_cms_simple() {
    CountMinBaseline* cms = new CountMinBaseline();
    cms->initialize(16, 4, 577);
    // Create packets where the first bytes are different (the rest of the packet src will be 0s)
    const char srcA[FT_SIZE] = {'A'};
    const char srcB[FT_SIZE] = {'B'};
    const char srcC[FT_SIZE] = {'C'};


    // Ratio A:B:C = 3:2:1
    for (int i = 0; i < 10000; i++) {
        cms->increment((char*) &srcA);
        cms->increment((char*) &srcA);
        cms->increment((char*) &srcA);

        cms->increment((char*) &srcB);
        cms->increment((char*) &srcB);

        cms->increment((char*) &srcC);
    }

    int cmsA = cms->query((char*) &srcA);
    int cmsB = cms->query((char*) &srcB);
    int cmsC = cms->query((char*) &srcC);
    printf("A: ");
    cms->print_indexes(srcA);
    printf("B: ");
    cms->print_indexes(srcB);
    printf("C: ");
    cms->print_indexes(srcC);

    if (cmsA != 30000 || cmsB != 20000 || cmsC != 10000) {
        printf("A: %i, B: %i, C: %i\n", cmsA, cmsB, cmsC);
        return -1;
    }

    return 0;
}

int test_flat_cms_simple() {
    CountMinFlat* cms = new CountMinFlat();
    cms->initialize(32, 5, 1);
    // Create packets where the first bytes are different (the rest of the packet src will be 0s)
    const char srcA[FT_SIZE] = {'A'};
    const char srcB[FT_SIZE] = {'B'};
    const char srcC[FT_SIZE] = {'C'};


    // Ratio A:B:C = 3:2:1
    for (int i = 0; i < 10000; i++) {
        cms->increment((char*) &srcA);
        cms->increment((char*) &srcA);
        cms->increment((char*) &srcA);

        cms->increment((char*) &srcB);
        cms->increment((char*) &srcB);

        cms->increment((char*) &srcC);
    }

    int cmsA = cms->query((char*) &srcA);
    int cmsB = cms->query((char*) &srcB);
    int cmsC = cms->query((char*) &srcC);

    printf("A: ");
    cms->print_indexes(srcA);
    printf("B: ");
    cms->print_indexes(srcB);
    printf("C: ");
    cms->print_indexes(srcC);

    if (cmsA != 30000 || cmsB != 20000 || cmsC != 10000) {
        printf("A: %i, B: %i, C: %i\n", cmsA, cmsB, cmsC);
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Missing arguments to program\n");
        return -1;
    }

    if (strcmp("test_baseline_cms_simple", argv[1]) == 0) {
        if (test_baseline_cms_simple() != 0) {
            printf("test baseline cms failed\n");
            return -1;
        }

        printf("test successful\n");
    } else if (strcmp("test_flat_cms_simple", argv[1]) == 0) {
        if (test_flat_cms_simple() != 0) {
            printf("test flat cms failed\n");
            return -1;
        }

        printf("test successful\n");
    } else if (strcmp("genzipf", argv[1]) == 0) {
        int seed = 879;
        const char* zipfPath = "zipf.data";
        genzipf(zipfPath, seed, 0.9, 1 << 24, 100000);
    } else {
            printf("Unrecognised command %s\n", argv[1]);
            return -1;
    }

    return 0;
}

