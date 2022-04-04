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

CountMinFlat::CountMinFlat(int k) {
  this->topK = new orderedMapTopK<int, uint32_t>(k);
}

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
    bobhash[i].initialize(seed * (7 + i) + i + 100);
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
  // for the synthetic data the first 4 bytes are enough to differentiate
  this->topK->update(*(int *)str, min);
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
  return binary_search_estimate_skew(this->counter, items.size(), items.begin(),
                                     items.end());
}

CountMinTopK::CountMinTopK(int k) {
  this->topK = new orderedMapTopK<int, uint32_t>(k);
}

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

  // for the synthetic data the first 4 bytes are enough to differentiate
  this->topK->update(*(int *)str, min);
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
