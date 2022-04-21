#pragma once

#ifndef OPTIMAL_PARAMETERS_HPP
#define OPTIMAL_PARAMETERS_HPP

#include <stdexcept>
#include <string>

/*
 * This file contains methods for getting the range of optimal parameters for
 * the flat count min variant based on previous experimentation.
 *
 */

enum ErrorMetric {
  normalized = 0,
  heavy_hitter = 1,
  sketch_e = 2,
  sketch_2e = 3,
  sketch_4e = 4,
  sketch_8e = 5
};

std::string error_metric_name(ErrorMetric metric);

void optimal_normalized_bounds(double skew, int *upper, int *lower);

void optimal_heavy_hitter_bounds(double skew, int *upper, int *lower);

void optimal_sketch_error_e_bounds(double skew, int *upper, int *lower);

void optimal_sketch_error_2e_bounds(double skew, int *upper, int *lower);

void optimal_sketch_error_4e_bounds(double skew, int *upper, int *lower);

void optimal_sketch_error_8e_bounds(double skew, int *upper, int *lower);

void optimal_bounds(double skew, int *upper, int *lower, ErrorMetric metric);

#endif
