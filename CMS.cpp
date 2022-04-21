// Code adapted from SALSA:
// https://github.com/SALSA-ICDE2021/SALSA/tree/main/Salsa

#include <assert.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <math.h>
#include <random>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "CMS.hpp"
#include "skew_estimation.hpp"

using namespace std;

CountMinBaseline::CountMinBaseline() {}

CountMinBaseline::~CountMinBaseline() {
  for (int i = 0; i < height; ++i) {
    delete[] baseline_cms[i];
  }
  delete[] bobhash;
  delete[] baseline_cms;
}

void CountMinBaseline::initialize(int width, int height, int seed) {
  this->width = width;
  this->height = height;

  width_mask = width - 1;

  assert(width > 0 && "We assume too much!");
  assert(width % 4 == 0 && "We assume that (w % 4 == 0)!");
  assert((width & (width - 1)) == 0 && "We assume that width is a power of 2!");

  baseline_cms = new uint32_t *[height];
  bobhash = new BOBHash[height];

  for (int i = 0; i < height; ++i) {
    baseline_cms[i] = new uint32_t[width]();
    bobhash[i].initialize(seed * (7 + i) + i + 100);
  }
}

void CountMinBaseline::increment(const char *str) {
  for (int i = 0; i < height; ++i) {
    uint index = (bobhash[i].run(str, FT_SIZE)) & width_mask;
    ++baseline_cms[i][index];
  }
}

uint64_t CountMinBaseline::query(const char *str) {
  uint index = (bobhash[0].run(str, FT_SIZE)) & width_mask;
  uint64_t min = baseline_cms[0][index];
  for (int i = 1; i < height; ++i) {
    uint index = (bobhash[i].run(str, FT_SIZE)) & width_mask;
    uint64_t temp = baseline_cms[i][index];
    if (min > temp) {
      min = temp;
    }
  }
  return min;
}

void CountMinBaseline::print_indexes(const char *str) {
  printf("H = [");
  for (int i = 0; i < height; ++i) {
    uint index = (bobhash[i].run(str, FT_SIZE)) & width_mask;
    if (i == 0) {
      printf("%i", index);
    } else {
      printf(", %i", index);
    }
  }
  printf("]\n");
}

CountMinBaselineFlexibleWidth::CountMinBaselineFlexibleWidth() {}

CountMinBaselineFlexibleWidth::~CountMinBaselineFlexibleWidth() {
  for (int i = 0; i < height; ++i) {
    delete[] baseline_cms[i];
  }
  delete[] bobhash;
  delete[] baseline_cms;
}

void CountMinBaselineFlexibleWidth::initialize(int width, int height,
                                               int seed) {
  this->width = width;
  this->height = height;

  assert(width > 0 && "width is greater than 0");

  baseline_cms = new uint32_t *[height];
  bobhash = new BOBHash[height];

  for (int i = 0; i < height; ++i) {
    baseline_cms[i] = new uint32_t[width]();
    bobhash[i].initialize(seed * (7 + i) + i + 100);
  }
}

void CountMinBaselineFlexibleWidth::increment(const char *str) {
  for (int i = 0; i < height; ++i) {
    uint index = (bobhash[i].run(str, FT_SIZE)) % width;
    ++baseline_cms[i][index];
  }
}

uint64_t CountMinBaselineFlexibleWidth::query(const char *str) {
  uint index = (bobhash[0].run(str, FT_SIZE)) % width;
  uint64_t min = baseline_cms[0][index];
  for (int i = 1; i < height; ++i) {
    uint index = (bobhash[i].run(str, FT_SIZE)) % width;
    uint64_t temp = baseline_cms[i][index];
    if (min > temp) {
      min = temp;
    }
  }
  return min;
}

CountMinFlat::CountMinFlat(int k) { this->topK = new TopK(k); }

CountMinFlat::~CountMinFlat() {
  delete[] flat_cms;
  delete[] bobhash;
}

void CountMinFlat::initialize(int width, int hash_count, int seed) {
  this->width = width;
  this->hash_count = hash_count;
  this->counter = 0;

  width_mask = width - 1;

  assert(width > 0 && "We assume too much!");
  assert(width % 4 == 0 && "We assume that (w % 4 == 0)!");
  assert((width & (width - 1)) == 0 && "We assume that width is a power of 2!");

  flat_cms = new uint32_t[width];
  bobhash = new BOBHash[hash_count];

  for (int i = 0; i < hash_count; ++i) {
    bobhash[i].initialize((seed * (3 + i) + i + 100) % 1229);
  }
}

void CountMinFlat::increment(const char *str) {
  uint32_t min = UINT32_MAX;
  for (int i = 0; i < hash_count; ++i) {
    uint index = (bobhash[i].run(str, FT_SIZE)) & width_mask;
    uint32_t val = ++flat_cms[index];
    if (val < min) {
      min = val;
    }
  }
  this->counter++;
  this->topK->update(str, min);
}

uint64_t CountMinFlat::query(const char *str) {
  uint64_t min = UINT64_MAX;
  for (int i = 0; i < hash_count; ++i) {
    uint index = (bobhash[i].run(str, FT_SIZE)) & width_mask;
    uint64_t temp = flat_cms[index];
    if (min > temp) {
      min = temp;
    }
  }
  return min;
}

double CountMinFlat::estimate_skew() {
  auto items = this->topK->items();
  return small_set_estimate_skew(this->counter, items.size(), items.begin(),
                                 items.end());
}

double CountMinFlat::sketch_error(double alpha, long total, int mem) {

  int threshold = (int)(alpha * (double)total / (double)mem);
  int above_threshold = 0;

  for (int counter = 0; counter < width; counter++) {
    if (this->flat_cms[counter] > threshold) {
      above_threshold++;
    }
  }

  double failure_prob = (double)above_threshold / (double)width;
  return failure_prob;
}

int CountMinFlat::get_hash_function_count() { return this->hash_count; }

CountMinTopK::CountMinTopK(int k) { this->topK = new TopK(k); }

CountMinTopK::~CountMinTopK() {
  for (int i = 0; i < height; ++i) {
    delete[] baseline_cms[i];
  }
  delete[] bobhash;
  delete[] baseline_cms;
}

void CountMinTopK::initialize(int width, int height, int seed) {
  this->width = width;
  this->height = height;

  assert(width > 0 && "We assume too much!");

  baseline_cms = new uint32_t *[height];
  bobhash = new BOBHash[height];

  for (int i = 0; i < height; ++i) {
    baseline_cms[i] = new uint32_t[width]();
    bobhash[i].initialize(seed * (7 + i) + i + 100);
  }
}

void CountMinTopK::increment(const char *str) {
  uint index = (bobhash[0].run(str, FT_SIZE)) % width;
  uint64_t min = baseline_cms[0][index];

  for (int i = 0; i < height; ++i) {
    uint index = (bobhash[i].run(str, FT_SIZE)) % width;
    uint64_t temp = ++baseline_cms[i][index];
    if (min > temp) {
      min = temp;
    }
  }

  this->counter++;
  this->topK->update(str, min);
}

uint64_t CountMinTopK::query(const char *str) {
  uint index = (bobhash[0].run(str, FT_SIZE)) % width;
  uint64_t min = baseline_cms[0][index];
  for (int i = 1; i < height; ++i) {
    uint index = (bobhash[i].run(str, FT_SIZE)) % width;
    uint64_t temp = baseline_cms[i][index];
    if (min > temp) {
      min = temp;
    }
  }
  return min;
}

double CountMinTopK::sketch_error(double alpha, long total, int mem) {
  long double failure_prob = 1.0;

  int threshold = (int)(alpha * (double)total / (double)mem);

  for (int row = 0; row < this->height; row++) {
    int aboveThreshold = 0;

    for (int counter = 0; counter < width; counter++) {
      if (this->baseline_cms[row][counter] > threshold) {
        aboveThreshold++;
      }
    }

    double row_prob = (double)aboveThreshold / (double)width;
    failure_prob *= (long double)row_prob;
  }
  return failure_prob;
}

double CountMinTopK::estimate_skew() {
  auto items = this->topK->items();
  return small_set_estimate_skew(this->counter, items.size(), items.begin(),
                                 items.end());
}
void CountMinTopK::print_indexes(const char *str) {
  printf("H = [");
  for (int i = 0; i < height; ++i) {
    uint index = (bobhash[i].run(str, FT_SIZE)) % width;
    if (i == 0) {
      printf("%i", index);
    } else {
      printf(", %i", index);
    }
  }
  printf("]\n");
}

int CountMinTopK::get_hash_function_count() { return this->height; }

DynamicCountMin::DynamicCountMin(int k, ErrorMetric metric) {
  this->topK = new TopK(k);
  this->optimisation_target = metric;
}

DynamicCountMin::~DynamicCountMin() {
  delete[] flat_cms;
  delete[] bobhash;
}

void DynamicCountMin::initialize(int width, int start_hash_count, int seed) {
  this->width = width;
  this->hash_count = start_hash_count;
  this->counter = 0;

  width_mask = width - 1;

  assert(width > 0 && "We assume too much!");
  assert(width % 4 == 0 && "We assume that (w % 4 == 0)!");
  assert((width & (width - 1)) == 0 && "We assume that width is a power of 2!");

  flat_cms = new uint32_t[width];
  bobhash = new BOBHash[start_hash_count];

  for (int i = 0; i < start_hash_count; ++i) {
    bobhash[i].initialize((seed * (3 + i) + i + 100) % 1229);
  }
}

void DynamicCountMin::increment(const char *str) {
  const int threshold = 1 << 17;
  uint32_t min = UINT32_MAX;
  for (int i = 0; i < hash_count; ++i) {
    uint index = (bobhash[i].run(str, FT_SIZE)) & width_mask;
    uint32_t val = ++flat_cms[index];
    if (val < min) {
      min = val;
    }
  }
  this->counter++;

  if (this->counter == threshold) {
    this->dynamic_reconfigure();
  }

  this->topK->update(str, min);
}

void DynamicCountMin::dynamic_reconfigure() {
  double skew = this->estimate_skew();

  int lower = 0;
  int upper = 0;
  optimal_bounds(skew, &upper, &lower, this->optimisation_target);
  // Currently we don't use upper but it might have a use if we want to
  // repeatedly dynamically configure as we can avoid going to too few hash
  // functions too quickly
  (void)upper;

  if (lower < this->hash_count) {
    printf("Dyanmic reconfigure from %d to %d (skew=%f)\n", this->hash_count,
           lower, skew);
    this->hash_count = lower;
  } else {
    printf("Unable to dyanmic reconfigure from %d to %d (skew=%f)\n",
           this->hash_count, lower, skew);
  }
}

uint64_t DynamicCountMin::query(const char *str) {
  uint64_t min = UINT64_MAX;
  for (int i = 0; i < hash_count; ++i) {
    uint index = (bobhash[i].run(str, FT_SIZE)) & width_mask;
    uint64_t temp = flat_cms[index];
    if (min > temp) {
      min = temp;
    }
  }
  return min;
}

double DynamicCountMin::estimate_skew() {
  auto items = this->topK->items();
  return small_set_estimate_skew(this->counter, items.size(), items.begin(),
                                 items.end());
}

double DynamicCountMin::sketch_error(double alpha, long total, int mem) {

  int threshold = (int)(alpha * (double)total / (double)mem);
  int above_threshold = 0;

  for (int counter = 0; counter < width; counter++) {
    if (this->flat_cms[counter] > threshold) {
      above_threshold++;
    }
  }

  double failure_prob = (double)above_threshold / (double)width;
  return failure_prob;
}

int DynamicCountMin::get_hash_function_count() { return this->hash_count; }
