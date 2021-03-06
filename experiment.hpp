#pragma once

#include "CMS.hpp"
#include "Counter.hpp"
#include "TraceReader.hpp"
#include "skew_estimation.hpp"
#include "topK.hpp"
#include "zipf_stats.hpp"

#include <algorithm>
#include <stdio.h>
#include <utility>
#include <vector>

#include <math.h>

// Mem = counters (1 counter = 4 bytes)
//
// Alpha is multiplied by e before use
void run_experiment_fixed_mem(char *zipfPath, FILE *results, int hashFunctions,
                              int mem, double alpha) {

  alpha *= exp(1.0);

  HashPacketCounter counter = HashPacketCounter(1 << 27);
  auto true_top_k = TopK(1000);

  ZipfReader *reader = new ZipfReader(zipfPath);
  CountMinBaselineFlexibleWidth sketch = CountMinBaselineFlexibleWidth();
  int width = mem / hashFunctions;
  sketch.initialize(width, hashFunctions, 40);

  long long sum_sq_err = 0;
  long long n = 0;

  while (true) {
    char dest[FT_SIZE] = {};
    int ret = reader->read_next_packet(dest);

    // Reached end
    if (ret < FT_SIZE) {
      break;
    }

    if (ret != FT_SIZE) {
      throw std::runtime_error(
          "Failed sanity check - read incorrect number of bytes");
    }

    sketch.increment(dest);
    int actual = counter.increment(dest);
    int countMin = sketch.query(dest);
    true_top_k.update(dest, actual);

    if (countMin < actual) {
      throw std::runtime_error(
          "Failed sanity check - count min undercounted true value");
    }

    sum_sq_err += (long long)(countMin - actual) * (countMin - actual);
    n++;
  }

  long double inv_n = 1.0 / (long double)n;
  long double normalized_error = inv_n * sqrt(inv_n * (long double)sum_sq_err);

  int threshold = n / 1000;

  long double heavy_hitter_err = 0.0;
  int i = 1;
  char *packet = new char[FT_SIZE];
  packet[12] = (char)255;

  heavy_hitter_err = sqrt(heavy_hitter_err / (long double)i);

  long double failure_prob = 1.0;

  threshold = (int)(alpha * (float)n / (float)mem);

  for (int row = 0; row < hashFunctions; row++) {
    int aboveThreshold = 0;

    for (int counter = 0; counter < width; counter++) {
      if (sketch.baseline_cms[row][counter] > threshold) {
        aboveThreshold++;
      }
    }

    double row_prob = (double)aboveThreshold / (double)width;
    failure_prob *= (long double)row_prob;
  }

  fprintf(results, "%d,%Lf,%Lf,%Lf\n", hashFunctions, normalized_error,
          heavy_hitter_err, failure_prob);

  delete reader;
}

void run_experiment(char *zipfPath, const char *resultsPath,
                    int hashFunctions) {
  int size = 128;
  const int maxSize = 1024 * 256;
  size = maxSize;

  printf("Writing n=%d to %s\n", hashFunctions, resultsPath);
  FILE *results = fopen(resultsPath, "w");
  fprintf(results, "memory,normalized error\n");

  PacketCounter *counter = new PacketCounter(1 << 27);
  while (size <= maxSize) {
    counter->reset();
    BOBHash hasher = BOBHash(177);
    ZipfReader *reader = new ZipfReader(zipfPath);
    CountMinBaselineFlexibleWidth *sketch = new CountMinBaselineFlexibleWidth();
    sketch->initialize(size / hashFunctions, hashFunctions, 40);

    long sum_sq_err = 0;

    long n = 0;

    while (true) {
      char dest[FT_SIZE] = {};
      int ret = reader->read_next_packet(dest);

      // Reached end
      if (ret < FT_SIZE) {
        break;
      }

      if (ret != FT_SIZE) {
        throw std::runtime_error(
            "Failed sanity check - read incorrect number of bytes");
      }

      sketch->increment(dest);
      int actual = counter->increment(dest);
      int countMin = sketch->query(dest);

      if (countMin < actual) {
        throw std::runtime_error(
            "Failed sanity check - count min undercounted true value");
      }

      sum_sq_err += (long)(countMin - actual) * (countMin - actual);
      n++;
    }

    double inv_n = 1.0 / (double)n;
    double normalized_error = inv_n * sqrt(inv_n * (double)sum_sq_err);

    fprintf(results, "%d,%f\n", size, normalized_error);
    printf("Normalized error %f, memory: %d\n", normalized_error, size);

    size *= 2;
  }

  fclose(results);
}

void run_experiment_top_k(FILE *frequency_f, FILE *cost_f,
                          FILE *harmonic_estimate_f, char *zipfPath, int mem,
                          int k, int hashFunctions) {
  PacketCounter *counter = new PacketCounter(1 << 27);

  ZipfReader *reader = new ZipfReader(zipfPath);
  CountMinTopK *sketch = new CountMinTopK(k);
  int width = mem / hashFunctions;
  sketch->initialize(width, hashFunctions, 40);

  int total = 0;
  while (true) {
    char dest[FT_SIZE] = {};
    int ret = reader->read_next_packet(dest);

    // Reached end
    if (ret < FT_SIZE) {
      break;
    }

    if (ret != FT_SIZE) {
      throw std::runtime_error(
          "Failed sanity check - read incorrect number of bytes");
    }

    sketch->increment(dest);
    int actual = counter->increment(dest);
    int countMin = sketch->query(dest);

    total++;

    if (countMin < actual) {
      throw std::runtime_error(
          "Failed sanity check - count min undercounted true value");
    }
  }

  auto topK = sketch->topK->items();

  double estimated_skew =
      binary_search_estimate_skew(total, k, topK.begin(), topK.end());
  printf("estimated skew: %f\n", estimated_skew);

  fprintf(frequency_f, "index,count,key,frequency\n");
  fprintf(harmonic_estimate_f, "index,estimate\n");
  int n = 0;
  double total_estimate = 0.0;

  int skew_start = 5;
  int skew_end = 15;

  fprintf(cost_f, "skew,cost\n");
  for (int skew = skew_start; skew <= skew_end; skew++) {
    double cost =
        cost_topk_skew(total, k, (double)skew * 0.1, topK.begin(), topK.end());
    fprintf(cost_f, "%.1f,%f\n", 0.1 * (float)skew, cost);
  }
}

void run_experiment_flat_top_k(FILE *skew_estimate, char *zipfPath, int mem,
                               int k, int hashFunctions,
                               int estimateFrequency) {
  ZipfReader *reader = new ZipfReader(zipfPath);
  CountMinFlat *sketch = new CountMinFlat(k);
  sketch->initialize(mem, hashFunctions, 40);
  fprintf(skew_estimate, "read packets,skew estimate\n");

  int next_check = 1;
  int i = 0;
  while (true) {
    char dest[FT_SIZE] = {};
    int ret = reader->read_next_packet(dest);

    // Reached end
    if (ret < FT_SIZE) {
      break;
    }

    if (ret != FT_SIZE) {
      throw std::runtime_error(
          "Failed sanity check - read incorrect number of bytes");
    }

    sketch->increment(dest);

    if (i == next_check) {
      double estimate = sketch->estimate_skew();
      fprintf(skew_estimate, "%d,%f\n", i, estimate);
      next_check *= 2;
    }

    i++;
  }
}
