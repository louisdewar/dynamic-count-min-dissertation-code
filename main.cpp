#include "CMS.hpp"
#include "genzipf.h"
#include <iostream>
#include <string.h>
#include "TraceReader.hpp"
#include <math.h>

#include "experiment.hpp"

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
    if (argc < 6) {
      printf("Missing arguments for genzipf [output_path] [number of packets] [skew] [seed]\n");
      return -1;
    }

    char* zipfPath = argv[2];
    int packetNumber = stoi(argv[3]);
    double skew = stod(argv[4]);
    int seed = stoi(argv[5]);
    genzipf(zipfPath, seed, skew, 1 << 26, packetNumber);
  } else if (strcmp("experiment", argv[1]) == 0) {
    if (argc < 4) {
      printf("Missing arguments to experiment\n");
      return -1;
    }


    int hashFunctions = 1;
    for (int i = 0; i < 5; i++) {
      std::string fileName = argv[3];
      fileName += "-";
      fileName += std::to_string(hashFunctions);
      fileName += ".csv";
      run_experiment(argv[2], fileName.c_str(), hashFunctions);
      hashFunctions *= 2;
    }
  } else {
    printf("Unrecognised command %s\n", argv[1]);
    return -1;
  }

  return 0;
}
