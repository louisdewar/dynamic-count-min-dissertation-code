#include "skew_estimation.hpp"

#include <limits>

double debuging_estimate_harmonic_number(
    int N, int k, double skew,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator start,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator end,
    FILE *output) {
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
  }
  assert(i == k);
  auto index = estimates.begin() + k / 2;
  std::nth_element(estimates.begin(), index, estimates.end());
  double median = *index;

  return median;
}

double estimate_harmonic_number(
    int N, int k, double skew,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator start,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator end) {
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
  }
  assert(i == k);
  auto index = estimates.begin() + k / 2;
  std::nth_element(estimates.begin(), index, estimates.end());
  double median = *index;

  return median;
}

double harmonic_number_variance(
    int N, int k, double skew,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator start,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator end) {
  int i = 0;

  double inv_total = 1.0 / (double)N;
  double inv_k = 1.0 / (double)k;

  std::vector<double> estimates;
  estimates.reserve(k);
  //  Loop in inverse order since largest is at the end
  for (auto it = --end; it >= start; --it) {
    i++;
    uint32_t count = it->second;
    double frequency = (double)count * inv_k;
    double val = frequency * pow((double)i, skew);
    double estimate = 1.0 / val;
    estimates.push_back(estimate);
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
}

double cost_topk_skew(
    int N, int k, double skew_d,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator start,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator end) {
  assert(skew_d < 1.8);
  return harmonic_number_variance(N, k, skew_d, start, end);
}

double cost_topk_skew_old(
    int N, int k, double skew, double harmonic_n,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator start,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator end) {
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

double small_set_estimate_skew(
    int N, int k,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator start,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator end) {
  int steps = 10;
  double skew_start = 0.5;
  double delta_skew = 0.1;

  double best_skew = 0.0;
  double best_skew_cost = std::numeric_limits<double>::infinity();

  for (int i = 0; i < steps; i++) {
    double skew = skew_start + i * delta_skew;

    double cost = cost_topk_skew(N, k, skew, start, end);

    if (cost < best_skew_cost) {
      best_skew = skew;
      best_skew_cost = cost;
    }
  }

  return best_skew;
}

double binary_search_estimate_skew(
    int N, int k,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator start,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator end) {
  const double skew_grad_delta = 0.0001;
  const int MAX_ITERS = 10;
  double bottom = 0.5;
  double top = 1.5;

  for (int i = 0; i < MAX_ITERS; i++) {
    double skew_f = (bottom + top) * 0.5;

    double cost = cost_topk_skew(N, k, skew_f, start, end);
    double cost_plus_delta =
        cost_topk_skew(N, k, skew_f + skew_grad_delta, start, end);

    if (cost < cost_plus_delta) {
      // Cost is decreasing with a decreasing skew
      top = skew_f;
    } else if (cost > cost_plus_delta) {
      // Skew is decreasing with an increasing skew
      bottom = skew_f;
    } else {
      return skew_f;
    }
  }

  return (bottom + top) * 0.5;
}
