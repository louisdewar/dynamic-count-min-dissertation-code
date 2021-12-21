#pragma once

#include "CMS.hpp"
#include "Counter.hpp"
#include "HashMap.hpp"
#include "TraceReader.hpp"
#include <stdio.h>

#include <math.h>

int test_baseline_cms_simple() {
  CountMinBaseline *cms = new CountMinBaseline();
  cms->initialize(16, 4, 30);
  // Create packets where the first bytes are different (the rest of the packet
  // src will be 0s)
  const char srcA[FT_SIZE] = {'A'};
  const char srcB[FT_SIZE] = {'B'};
  const char srcC[FT_SIZE] = {'C'};

  // Ratio A:B:C = 3:2:1
  for (int i = 0; i < 10000; i++) {
    cms->increment((char *)&srcA);
    cms->increment((char *)&srcA);
    cms->increment((char *)&srcA);

    cms->increment((char *)&srcB);
    cms->increment((char *)&srcB);

    cms->increment((char *)&srcC);
  }

  int cmsA = cms->query((char *)&srcA);
  int cmsB = cms->query((char *)&srcB);
  int cmsC = cms->query((char *)&srcC);
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
  CountMinFlat *cms = new CountMinFlat();
  cms->initialize(32, 5, 1);
  // Create packets where the first bytes are different (the rest of the packet
  // src will be 0s)
  const char srcA[FT_SIZE] = {'A'};
  const char srcB[FT_SIZE] = {'B'};
  const char srcC[FT_SIZE] = {'C'};

  // Ratio A:B:C = 3:2:1
  for (int i = 0; i < 10000; i++) {
    cms->increment((char *)&srcA);
    cms->increment((char *)&srcA);
    cms->increment((char *)&srcA);

    cms->increment((char *)&srcB);
    cms->increment((char *)&srcB);

    cms->increment((char *)&srcC);
  }

  int cmsA = cms->query((char *)&srcA);
  int cmsB = cms->query((char *)&srcB);
  int cmsC = cms->query((char *)&srcC);

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

// Mem = counters (1 counter = 4 bytes)
//
// Alpha is multiplied by e before use
void run_experiment_fixed_mem(char *zipfPath, FILE *results, int hashFunctions,
                              int mem, double alpha) {

  alpha *= exp(1.0);

  PacketCounter *counter = new PacketCounter(1 << 26);

  ZipfReader *reader = new ZipfReader(zipfPath);
  CountMinBaseline *sketch = new CountMinBaseline();
  int width = mem / hashFunctions;
  sketch->initialize(width, hashFunctions, 40);

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

    sketch->increment(dest);
    int actual = counter->increment(dest);
    int countMin = sketch->query(dest);

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

  printf("Calculating heavy hitters for hash functions = %i\n", hashFunctions);
  while (counter->query_index(i) > threshold) {
    int *packetCast = (int *)packet;

    packetCast[0] = i;
    packetCast[1] = i;
    packetCast[2] = i;

    int actual = counter->query_index(i);
    int countMin = sketch->query(packet);

    long double err = (long double)actual - (long double)countMin;
    heavy_hitter_err += err * err;

    assert(i <= n / threshold);
    i++;
  }

  heavy_hitter_err = sqrt(heavy_hitter_err / (long double)i);

  // double alpha = 2.0 * exp(1.0);
  long double failure_prob = 1.0;

  threshold = (int)(alpha * (float)n / (float)mem);

  for (int row = 0; row < hashFunctions; row++) {
    int aboveThreshold = 0;

    for (int counter = 0; counter < width; counter++) {
      if (sketch->baseline_cms[row][counter] > threshold) {
        aboveThreshold++;
      }
    }

    double row_prob = (double)aboveThreshold / (double)width;
    failure_prob *= (long double)row_prob;
  }

  fprintf(results, "%d,%Lf,%Lf,%Lf\n", hashFunctions, normalized_error,
          heavy_hitter_err, failure_prob);
}

void run_experiment(char *zipfPath, const char *resultsPath,
                    int hashFunctions) {
  int size = 128;
  const int maxSize = 1024 * 256;
  size = maxSize;
  // const int maxSize = 16 * 1024 * 1024;

  printf("Writing n=%d to %s\n", hashFunctions, resultsPath);
  FILE *results = fopen(resultsPath, "w");
  fprintf(results, "memory,normalized error\n");

  // size_t cap = 1 << 28;
  // HashMapCounter* counter = new HashMapCounter(cap, 101);
  PacketCounter *counter = new PacketCounter(1 << 26);
  // unordered_map<char[FT_SIZE], long, PacketHash> counter;
  while (size <= maxSize) {
    counter->reset();
    BOBHash hasher = BOBHash(177);
    ZipfReader *reader = new ZipfReader(zipfPath);
    CountMinBaseline *sketch = new CountMinBaseline();
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
      // int actual = ++(counter[dest]);
      int actual = counter->increment(dest);
      int countMin = sketch->query(dest);

      // int hash = hasher.run(dest, FT_SIZE);

      // printf("debug, n: %d (hash=%d), mem: %d - before: %d, counted: %d,
      // before: %d, actual: %d\n", n, hash, size, beforeCountMin, countMin,
      // beforeActual, actual);

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
