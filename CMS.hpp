// Code adapted from SALSA:
// https://github.com/SALSA-ICDE2021/SALSA/tree/main/Salsa
//
// CountMinBaseline is from SALSA, with the others adapted from this one.

#pragma once

#ifndef COUNT_MIN_SKETCH
#define COUNT_MIN_SKETCH

#include <assert.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <math.h>
#include <random>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "BobHash.hpp"
#include "Defs.hpp"
#include "optimal_parameters.hpp"
#include "topK.hpp"

using namespace std;

class EvaluatableSketch {
public:
  virtual void increment(const char *str) = 0;
  virtual uint64_t query(const char *str) = 0;
  virtual double estimate_skew() = 0;
  virtual double sketch_error(double alpha, long total, int mem) = 0;
  virtual int get_hash_function_count() = 0;
};

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

  BOBHash *bobhash;

public:
  int height;
  uint32_t **baseline_cms;

  CountMinBaselineFlexibleWidth();
  ~CountMinBaselineFlexibleWidth();

  void initialize(int width, int height, int seed);
  void increment(const char *str);
  uint64_t query(const char *str);
};

class CountMinFlat : public EvaluatableSketch {

  int width;
  int counter;

  int width_mask;

  BOBHash *bobhash;

  uint32_t *flat_cms;

public:
  int hash_count;
  TopK *topK;

  CountMinFlat(int k);
  ~CountMinFlat();

  void initialize(int width, int hash_count, int seed);
  void increment(const char *str);
  uint64_t query(const char *str);

  double estimate_skew();
  double sketch_error(double alpha, long total, int mem);
  int get_hash_function_count();
};

class CountMinTopK : public EvaluatableSketch {

  int width;

  BOBHash *bobhash;
  int counter;

public:
  int height;
  TopK *topK;
  uint32_t **baseline_cms;

  // We keep track of the k top elements.
  CountMinTopK(int k);
  ~CountMinTopK();

  void initialize(int width, int height, int seed);
  void increment(const char *str);
  uint64_t query(const char *str);

  void print_indexes(const char *str);
  double estimate_skew();
  double sketch_error(double alpha, long total, int mem);
  int get_hash_function_count();
};

class DynamicCountMin : public EvaluatableSketch {
  int width;
  int counter;
  bool use_bounds;

  int width_mask;

  BOBHash *bobhash;

  uint32_t *flat_cms;

  ErrorMetric optimisation_target;

  void dynamic_reconfigure();

public:
  int hash_count;
  TopK *topK;

  // set `use_bounds` to use the error bounds, otherwise it uses the lowest
  // average error configuration.
  DynamicCountMin(int k, ErrorMetric optimisation_target, bool use_bounds);
  ~DynamicCountMin();

  void initialize(int width, int hash_count, int seed);
  void increment(const char *str);
  uint64_t query(const char *str);

  double estimate_skew();
  double sketch_error(double alpha, long total, int mem);
  int get_hash_function_count();
};
#endif
