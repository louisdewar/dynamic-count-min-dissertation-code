#include "CMS.hpp"
#include "TraceReader.hpp"
#include "genzipf.h"
#include <iostream>
#include <stdio.h>
#include <string.h>

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
      printf("Missing arguments for genzipf [output_path] [number of packets] "
             "[skew] [seed]\n");
      return -1;
    }

    char *zipfPath = argv[2];
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
    for (int i = 0; i < 4; i++) {
      std::string fileName = argv[3];
      fileName += "-";
      fileName += std::to_string(hashFunctions);
      fileName += ".csv";
      run_experiment(argv[2], fileName.c_str(), hashFunctions);
      hashFunctions *= 2;
    }
  } else if (strcmp("test_traces_fixed_mem", argv[1]) == 0) {
    if (argc < 6) {
      printf("Missing arguments to experiment\n");
      return -1;
    }

    int mem = stoi(argv[4]);
    float alpha = stod(argv[5]);

    std::string fileName = argv[3];
    FILE *results = fopen(fileName.c_str(), "w");
    fprintf(
        results,
        "hash functions,normalized error,heavy hitter error,sketch error\n");
    int hashFunctions[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int len = 10;
    for (int i = 0; i < len; i++) {
      run_experiment_fixed_mem(argv[2], results, hashFunctions[i], mem, alpha);
    }
  } else if (strcmp("test_trace_fixed_alpha", argv[1]) == 0) {
    if (argc < 6) {
      printf("Missing arguments to experiment\n");
      return -1;
    }

    char *trace = argv[2];
    char *csv_output_path = argv[3];
    double alpha = stod(argv[4]);
    int mem = stoi(argv[5]);

    FILE *results = fopen(csv_output_path, "w");
    fprintf(
        results,
        "hash functions,normalized error,heavy hitter error,sketch error\n");
    int len = 10;
    int hashFunctions[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    for (int i = 0; i < len; i++) {
      run_experiment_fixed_mem(trace, results, hashFunctions[i], mem, alpha);
    }
  } else if (strcmp("test_top_k", argv[1]) == 0) {
    if (argc < 9) {
      printf("Missing arguments to experiment\n");
      return -1;
    }

    char *trace = argv[2];
    char *histogram_output_path = argv[3];
    char *skew_cost_output_path = argv[4];
    char *harmonic_estimate_output_path = argv[5];
    int k = stoi(argv[6]);
    int mem = stoi(argv[7]);
    int hashFunctions = stoi(argv[8]);

    FILE *histogram = fopen(histogram_output_path, "w");
    FILE *skew_cost = fopen(skew_cost_output_path, "w");
    FILE *harmonic_estimate = fopen(harmonic_estimate_output_path, "w");

    run_experiment_top_k(histogram, skew_cost, harmonic_estimate, trace, mem, k,
                         hashFunctions);
  } else {
    printf("Unrecognised command %s\n", argv[1]);
    return -1;
  }

  return 0;
}
