
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
  case sketch_e:
    return "sketch e";
  case sketch_2e:
    return "sketch 2e";
  case sketch_4e:
    return "sketch 4e";
  case sketch_8e:
    return "sketch 8e";
  }
  throw std::runtime_error("invalid error metric");
}

void optimal_normalized_bounds(double skew, int *upper, int *lower) {
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

void optimal_heavy_hitter_bounds(double skew, int *upper, int *lower) {
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

void optimal_sketch_error_e_bounds(double skew, int *upper, int *lower) {
  *upper = 1;
  *lower = 1;
}

void optimal_sketch_error_2e_bounds(double skew, int *upper, int *lower) {
  *upper = 1;
  *lower = 1;
}

void optimal_sketch_error_4e_bounds(double skew, int *upper, int *lower) {
  *upper = 1;
  *lower = 1;
}

void optimal_sketch_error_8e_bounds(double skew, int *upper, int *lower) {
  *upper = 1;
  *lower = 1;
}

void optimal_bounds(double skew, int *upper, int *lower, ErrorMetric metric) {
  switch (metric) {
  case normalized:
    optimal_normalized_bounds(skew, upper, lower);
    break;
  case heavy_hitter:
    optimal_heavy_hitter_bounds(skew, upper, lower);
    break;
  case sketch_e:
    optimal_sketch_error_e_bounds(skew, upper, lower);
    break;
  case sketch_2e:
    optimal_sketch_error_2e_bounds(skew, upper, lower);
    break;
  case sketch_4e:
    optimal_sketch_error_4e_bounds(skew, upper, lower);
    break;
  case sketch_8e:
    optimal_sketch_error_8e_bounds(skew, upper, lower);
    break;
  }
}
