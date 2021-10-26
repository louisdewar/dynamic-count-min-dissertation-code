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

  uint32_t **baseline_cms;

public:
  CountMinBaseline();
  ~CountMinBaseline();

  void initialize(int width, int height, int seed);
  void increment(const char *str);
  uint64_t query(const char *str);

  void print_indexes(const char *str);
};

class CountMinFlat {

  int width;
  int hash_count;

  int width_mask;

  BOBHash *bobhash;

  uint32_t *flat_cms;

public:
  CountMinFlat();
  ~CountMinFlat();

  void initialize(int width, int hash_count, int seed);
  void increment(const char *str);
  uint64_t query(const char *str);

  void print_indexes(const char *str);
};

#endif // !COUNT_MIN_SKETCH
