
#include "optimal_parameters.hpp"

/*
 * Returns the optimal number of hash functions for the flat count min depending
 * on the error metric.
 * The optimal is based on empirical data from the synthetic skews.
 * */

std::string error_metric_name(ErrorMetric metric) {
  switch (metric) {
  case normalized:
    return "normalized";
  case heavy_hitter:
    return "heavy hitter";
  }
  throw std::runtime_error("invalid error metric");
}

void optimal_normalized_bounds(double skew, int *upper, int *lower, int *best) {
  /*
   * skew,upper,lower
   * 0.6,1,1
   * 0.7,1,2
   * 0.8,2,2
   * 0.9,2,2
   * 1.0,2,2
   * 1.1,2,3
   * 1.2,2,3
   * 1.3,2,3
   */
  /*
   * skew,hash functions,normalized-error
   * 0.6,1,1.2092004e-14
   * 0.7,2,1.5519404e-14
   * 0.8,2,1.3614118e-14
   * 0.9,2,1.0728066e-14
   * 1.0,2,7.240253400000001e-15
   * 1.1,3,3.9562842e-15
   * 1.2,3,1.436481e-15
   * 1.3,3,4.478922400000001e-16
   */

  if (skew < 0.65) {
    *best = 1;
  } else if (skew < 1.05) {
    *best = 2;
  } else {
    *best = 3;
  }

  if (skew < 0.65) {
    *lower = 1;
    *upper = 1;
  } else if (skew < 0.75) {
    *lower = 1;
    *upper = 2;
  } else if (skew < 1.05) {
    *lower = 2;
    *upper = 2;
  } else {
    *lower = 2;
    *upper = 3;
  }
}

void optimal_heavy_hitter_bounds(double skew, int *upper, int *lower,
                                 int *best) {
  /*
   * skew,upper,lower
   * 0.6,1,1
   * 0.7,1,1
   * 0.8,1,2
   * 0.9,1,2
   * 1.0,1,2
   * 1.1,1,2
   * 1.2,1,2
   * 1.3,2,2
   */
  /*
   * skew,hash functions,heavy-hitter-error
   * 0.6,1,0.0
   * 0.7,1,7.839999999999999e-07
   * 0.8,1,6.593492600000001e-07
   * 0.9,1,6.9037496e-07
   * 1.0,1,5.60573e-07
   * 1.1,1,3.2302442e-07
   * 1.2,2,1.4968696e-07
   * 1.3,2,6.0486998e-08
   */

  if (skew < 1.15) {
    *best = 1;
  } else {
    *best = 2;
  }

  if (skew < 0.75) {
    *lower = 1;
    *upper = 1;
  } else if (skew < 1.25) {
    *lower = 1;
    *upper = 2;
  } else {
    *lower = 2;
    *upper = 2;
  }
}

void optimal_bounds(double skew, int *upper, int *lower, int *best,
                    ErrorMetric metric) {
  switch (metric) {
  case normalized:
    optimal_normalized_bounds(skew, upper, lower, best);
    break;
  case heavy_hitter:
    optimal_heavy_hitter_bounds(skew, upper, lower, best);
    break;
  }
}
