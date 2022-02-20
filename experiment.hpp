#pragma once

#include "CMS.hpp"
#include "Counter.hpp"
#include "HashMap.hpp"
#include "TraceReader.hpp"
#include "topK.hpp"
#include "zipf_stats.hpp"
#include <algorithm>
#include <stdio.h>
#include <utility>
#include <vector>

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
  CountMinFlat cms = CountMinFlat();
  cms.initialize(32, 5, 1);
  // Create packets where the first bytes are different (the rest of the packet
  // src will be 0s)
  const char srcA[FT_SIZE] = {'A'};
  const char srcB[FT_SIZE] = {'B'};
  const char srcC[FT_SIZE] = {'C'};

  // Ratio A:B:C = 3:2:1
  for (int i = 0; i < 10000; i++) {
    cms.increment((char *)&srcA);
    cms.increment((char *)&srcA);
    cms.increment((char *)&srcA);

    cms.increment((char *)&srcB);
    cms.increment((char *)&srcB);

    cms.increment((char *)&srcC);
  }

  int cmsA = cms.query((char *)&srcA);
  int cmsB = cms.query((char *)&srcB);
  int cmsC = cms.query((char *)&srcC);

  printf("A: ");
  cms.print_indexes(srcA);
  printf("B: ");
  cms.print_indexes(srcB);
  printf("C: ");
  cms.print_indexes(srcC);

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

  HashPacketCounter counter = HashPacketCounter(1 << 27);
  auto true_top_k = orderedMapTopK<int, uint32_t>(1000);

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
    true_top_k.update(*(int *)&dest, actual);

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

  // printf("Calculating heavy hitters for hash functions = %i\n",
  // hashFunctions); while (counter.query_index(i) > threshold) {
  //   // for (auto item: true_top_k.items()) {
  //   int *packetCast = (int *)packet;

  //   packetCast[0] = i;
  //   packetCast[1] = i;
  //   packetCast[2] = i;

  //   int actual = counter.query_index(i);
  //   int countMin = sketch.query(packet);

  //   long double err = (long double)actual - (long double)countMin;
  //   heavy_hitter_err += err * err;

  //   assert(i <= n / threshold);
  //   i++;
  // }

  heavy_hitter_err = sqrt(heavy_hitter_err / (long double)i);

  // double alpha = 2.0 * exp(1.0);
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
  // const int maxSize = 16 * 1024 * 1024;

  printf("Writing n=%d to %s\n", hashFunctions, resultsPath);
  FILE *results = fopen(resultsPath, "w");
  fprintf(results, "memory,normalized error\n");

  // size_t cap = 1 << 28;
  // HashMapCounter* counter = new HashMapCounter(cap, 101);
  PacketCounter *counter = new PacketCounter(1 << 27);
  // unordered_map<char[FT_SIZE], long, PacketHash> counter;
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

double debuging_estimate_harmonic_number(
    int N, int k, double skew, std::vector<pair<int, uint32_t>>::iterator start,
    std::vector<pair<int, uint32_t>>::iterator end, FILE *output) {
  assert(skew < 1.5);

  int i = 0;

  double inv_total = 1.0 / (double)N;

  std::vector<double> estimates;
  //  Loop in inverse order since largest is at the end
  for (auto it = --end; it >= start; --it) {
    i++;
    uint32_t count = it->second;
    double frequency = (double)count * inv_total;
    double val = frequency * pow((double)i, skew);
    double estimate = 1.0 / val;
    estimates.push_back(estimate);
    fprintf(output, "%i,%f\n", i, estimate);
    // printf("estimate (skew=%f,i=%i, pow(i, skew)=%f): %f\n", skew, i,
    //        pow((double)i, skew), estimate);
  }
  assert(i == k);
  auto index = estimates.begin() + k / 2;
  std::nth_element(estimates.begin(), index, estimates.end());
  double median = *index;
  // printf("Mean estimate of harmonic f (skew=%f): %f\n", skew,
  // total_estimate);

  return median;
}

double
estimate_harmonic_number(int N, int k, double skew,
                         std::vector<pair<int, uint32_t>>::iterator start,
                         std::vector<pair<int, uint32_t>>::iterator end) {
  assert(skew < 1.5);
  int i = 0;

  double inv_total = 1.0 / (double)N;
  // double inv_k = 1.0 / (double)k;

  std::vector<double> estimates;
  //  Loop in inverse order since largest is at the end
  for (auto it = --end; it >= start; --it) {
    i++;
    uint32_t count = it->second;
    double frequency = (double)count * inv_total;
    double val = frequency * pow((double)i, skew);
    double estimate = 1.0 / val;
    estimates.push_back(estimate);
    // printf("estimate (skew=%f,i=%i, pow(i, skew)=%f): %f\n", skew, i,
    //        pow((double)i, skew), estimate);
  }
  assert(i == k);
  auto index = estimates.begin() + k / 2;
  std::nth_element(estimates.begin(), index, estimates.end());
  double median = *index;
  // printf("Mean estimate of harmonic f (skew=%f): %f\n", skew,
  // total_estimate);

  return median;
}

double
harmonic_number_variance(int N, int k, double skew,
                         std::vector<pair<int, uint32_t>>::iterator start,
                         std::vector<pair<int, uint32_t>>::iterator end) {
  assert(skew < 1.8);
  int i = 0;

  // This algorithm is potentially vulnerable to floating point cancellation.

  double inv_total = 1.0 / (double)N;
  double inv_k = 1.0 / (double)k;

  double avr_sum_squares = 0.0;
  double avr_sum = 0.0;

  std::vector<double> estimates;
  estimates.reserve(k);
  //  Loop in inverse order since largest is at the end
  for (auto it = --end; it >= start; --it) {
    i++;
    uint32_t count = it->second;
    double frequency = (double)count * inv_total;
    double val = frequency * pow((double)i, skew);
    double estimate = 1.0 / val;
    // avr_sum += inv_k * estimate;
    // avr_sum_squares += inv_k * estimate * estimate;
    estimates.push_back(estimate);
    //  printf("estimate (skew=%f,i=%i, pow(i, skew)=%f): %f\n", skew, i,
    //         pow((double)i, skew), estimate);
  }
  assert(i == k);
  double sum = 0.0;

  for (i = 0; i < k; i++) {
    sum += estimates[i];
  }

  double mean = sum / (double)k;

  double var = 0.0;
  for (i = 0; i < k; i++) {
    var += (estimates[i] - mean) * (estimates[i] - mean);
  }
  return var;

  // return avr_sum_squares - avr_sum;

  // auto index = estimates.begin() + k / 2;
  // std::nth_element(estimates.begin(), index, estimates.end());
  // double median = *index;
  // printf("Mean estimate of harmonic f (skew=%f): %f\n", skew,
  // total_estimate);
}

double cost_topk_skew(int N, int k, double skew_d,
                      std::vector<pair<int, uint32_t>>::iterator start,
                      std::vector<pair<int, uint32_t>>::iterator end) {
  return harmonic_number_variance(N, k, skew_d, start, end);
  // double cost = 0.0;
  // double harmonic_n = estimate_harmonic_number(N, k, skew_d, start, end);
  // printf("Skew: %f, harmonic_n: %f\n", skew_d, harmonic_n);
  // // double inv_harmonic_n = 1.0 / get_harmonic_number_or_calc(1 << 26,
  // skew); double inv_harmonic_n = 1.0 / harmonic_n; double inv_n = 1.0 /
  // (double)N;

  // int i = 0;
  // //  Loop in inverse order since largest is at the end
  // for (auto it = end; it != start; --it) {
  //   i++;
  //   uint32_t count = it->second;
  //   // printf("i: %d, count: %d\n", i, count);
  //   double expected = inv_harmonic_n / (pow((double)i, skew_d));
  //   double actual = inv_n * (double)count;
  //   // printf("Frequency for %i: %f (expected=%f), inv_harmonic_n=%f, pow(i,
  //   // skew)=%f\n", i, actual, expected, inv_harmonic_n, pow((double)i,
  //   skew));

  //   double diff = expected - actual;
  //   cost += diff * diff;
  // }

  // assert(i == k);

  // return cost / (float)k;
}

double cost_topk_skew_old(int N, int k, double skew, double harmonic_n,
                          std::vector<pair<int, uint32_t>>::iterator start,
                          std::vector<pair<int, uint32_t>>::iterator end) {
  // printf("Skew: %f\n", skew);
  double cost = 0.0;
  // double inv_harmonic_n = 1.0 / get_harmonic_number_or_calc(1 << 26, skew);
  double inv_harmonic_n = 1.0 / harmonic_n;
  double inv_n = 1.0 / (double)N;

  int i = 0;
  //  Loop in inverse order since largest is at the end
  for (auto it = end; it != start; --it) {
    i++;
    uint32_t count = it->second;
    // printf("i: %d, count: %d\n", i, count);
    double expected = inv_harmonic_n / (pow((double)i, skew));
    double actual = inv_n * (double)count;
    // printf("Frequency for %i: %f (expected=%f), inv_harmonic_n=%f, pow(i,
    // skew)=%f\n", i, actual, expected, inv_harmonic_n, pow((double)i, skew));

    double diff = expected - actual;
    cost += diff * diff;
  }

  assert(i == k);

  return cost / (float)k;
}

double estimate_skew(int N, int k,
                     std::vector<pair<int, uint32_t>>::iterator start,
                     std::vector<pair<int, uint32_t>>::iterator end) {
  const double skew_grad_delta = 0.0001;
  const int MAX_ITERS = 10;
  double bottom = 0.5;
  double top = 1.4;

  for (int i = 0; i < MAX_ITERS; i++) {
    double skew_f = (bottom + top) * 0.5;
    // int skew_i = floor(skew_f * 10.0 + 0.5);

    // TODO: replace with cost_topk_skew which will estimate the harmonic number
    // double harmonic = calculate_harmonic_number(1 << 26, skew_f);
    // double cost = cost_topk_skew_old(N, k, skew_f, harmonic, start, end);
    // double harmonic = estimate_harmonic_number(N, k, skew_f, start, end);
    // double cost = cost_topk_skew_old(N, k, skew_f, harmonic, start, end);
    // harmonic =
    //     estimate_harmonic_number(N, k, skew_f + skew_grad_delta, start, end);
    // double cost_plus_delta = cost_topk_skew_old(N, k, skew_f +
    // skew_grad_delta,
    //                                             harmonic, start, end);
    // harmonic = estimate_harmonic_number(N, k, skew_f + 0.1, start, end);
    // double cost_plus_big_delta =
    //     cost_topk_skew_old(N, k, skew_f + 0.1, harmonic, start, end);

    double cost = cost_topk_skew(N, k, skew_f, start, end);
    double cost_plus_delta =
        cost_topk_skew(N, k, skew_f + skew_grad_delta, start, end);

    // printf("for skew: %f, cost: %f, cost_plus_delta: %f, \n", skew_f, cost,
    //        cost_plus_delta);
    if (cost < cost_plus_delta) {
      // Cost is decreasing with a decreasing skew
      top = skew_f;
    } else if (cost > cost_plus_delta) {
      // Skew is decreasing with an increasing skew
      bottom = skew_f;
    } else {
      // printf(
      //     "skew cost did not change between %f and %f (iter = %i)
      //     (cost=%f)\n", skew_f, skew_f + skew_grad_delta, i, cost);
      return skew_f;
    }
  }

  return (bottom + top) * 0.5;
}

void run_experiment_top_k(FILE *frequency_f, FILE *cost_f,
                          FILE *harmonic_estimate_f, char *zipfPath, int mem,
                          int k, int hashFunctions) {
  // print_cached_skews();

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

  // auto topK = sketch->topK->items();
  std::vector<std::pair<int, uint32_t>> topK = {};

  // for (int i = k; i > 0; i--) {
  //   uint32_t count = counter->query_index(i);
  //   topK.push_back(std::make_pair(i, count));
  // }

  // Overwrite with estimates
  topK = sketch->topK->items();

  // auto begin = std::begin(*topK);
  // auto end = std::end(*topK);

  double estimated_skew = estimate_skew(total, k, topK.begin(), topK.end());
  printf("estimated skew: %f\n", estimated_skew);

  fprintf(frequency_f, "index,count,key,frequency\n");
  fprintf(harmonic_estimate_f, "index,estimate\n");
  int n = 0;
  double total_estimate = 0.0;

  // printf("Total: %d\n", total);

  // double harmonic_number = get_harmonic_number_or_calc(1 << 26, 9);
  // printf("Using harmonic number %f for skew 0.9\n", harmonic_number);
  // // It's given in the order of smallest to largest, but we want largest to
  // // smallest
  // for (unsigned i = topK.size() - 1; topK.size() > i; --i) {
  //   auto count = topK[i];
  //   double frequency = (float)count.second / (float)total;
  //   fprintf(frequency_f, "%i,%d,%d,%f\n", n, count.second, count.first,
  //           frequency);
  //   double test_skew = 0.9;
  //   double estimate =
  //       estimate_harmonic_number(total, k, 0.9, topK.begin(), topK.end());
  //   fprintf(harmonic_estimate_f, "%i,%f\n", n, estimate);
  //   total_estimate += estimate / (double)k;
  //   n++;
  // }

  // Print the harmonic number estimates for skew = 0.9
  // debuging_estimate_harmonic_number(total, k, 0.7, topK.begin(), topK.end(),
  //                                   harmonic_estimate_f);

  int skew_start = 5;
  int skew_end = 15;

  fprintf(cost_f, "skew,cost\n");
  for (int skew = skew_start; skew <= skew_end; skew++) {
    // double harmonic_n = get_harmonic_number_or_calc(1 << 26, skew);
    //  printf("For skew %d, harmonic number: %f\n", skew, harmonic_n);

    // printf("calculating top k cost for skew %.1f\n", 0.1 * (float)skew);
    // double cost = cost_topk_skew_old(total, k, (double)skew * 0.1,
    // harmonic_n,
    //                                  topK.begin(), topK.end());
    double cost =
        cost_topk_skew(total, k, (double)skew * 0.1, topK.begin(), topK.end());
    // printf("for skew %.1f, cost = %f\n", 0.1 * (float)skew, cost);
    //  double cost = cost_topk_skew(total, k, skew, topK.begin(), topK.end());
    fprintf(cost_f, "%.1f,%f\n", 0.1 * (float)skew, cost);
  }

  // fprintf(results, "%d,%Lf,%Lf,%Lf\n", hashFunctions, normalized_error,
  //         heavy_hitter_err, failure_prob);
}
