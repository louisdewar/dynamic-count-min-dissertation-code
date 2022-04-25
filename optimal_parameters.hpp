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
};

std::string error_metric_name(ErrorMetric metric);

void optimal_normalized_bounds(double skew, int *upper, int *lower, int *best);

void optimal_heavy_hitter_bounds(double skew, int *upper, int *lower,
                                 int *best);

void optimal_bounds(double skew, int *upper, int *lower, int *best,
                    ErrorMetric metric);

#endif
