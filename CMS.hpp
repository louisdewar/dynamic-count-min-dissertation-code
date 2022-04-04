#pragma once

#ifndef COUNT_MIN_SKETCH
#define COUNT_MIN_SKETCH

#include <assert.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <math.h>
#include <random>
#include <stdlib.h>
#include <time.h>

#include "BobHash.hpp"
#include "Defs.hpp"
#include "topK.hpp"

using namespace std;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

class CountMinBaseline {

  int width;
  int height;

  int width_mask;

  BOBHash *bobhash;

public:
  uint32_t **baseline_cms;

  CountMinBaseline();
  ~CountMinBaseline();

  void initialize(int width, int height, int seed);
  void increment(const char *str);
  uint64_t query(const char *str);

  void print_indexes(const char *str);
};

/// A version of count min baseline where the width does not need to be a power
/// of 2 (NOTE: this makes indexing less efficient)
class CountMinBaselineFlexibleWidth {

  int width;
  int height;

  BOBHash *bobhash;

public:
  uint32_t **baseline_cms;

  CountMinBaselineFlexibleWidth();
  ~CountMinBaselineFlexibleWidth();

  void initialize(int width, int height, int seed);
  void increment(const char *str);
  uint64_t query(const char *str);
};

class CountMinFlat {

  int width;
  int hash_count;
  int counter;

  int width_mask;

  BOBHash *bobhash;

  uint32_t *flat_cms;

public:
  // TODO: change key to std::array or maybe custom type without need for a map
  orderedMapTopK<int, uint32_t> *topK;

  CountMinFlat(int k);
  ~CountMinFlat();

  void initialize(int width, int hash_count, int seed);
  void increment(const char *str);
  uint64_t query(const char *str);

  double estimate_skew();
};

class CountMinTopK {

  int width;
  int height;

  BOBHash *bobhash;

public:
  // TODO: change to std::array or maybe custom type without need for a map
  orderedMapTopK<int, uint32_t> *topK;
  uint32_t **baseline_cms;

  // We keep track of the k top elements.
  CountMinTopK(int k);
  ~CountMinTopK();

  void initialize(int width, int height, int seed);
  void increment(const char *str);
  uint64_t query(const char *str);

  void print_indexes(const char *str);
};

#endif // !COUNT_MIN_SKETCH
