#include "skew_estimation.hpp"

double debuging_estimate_harmonic_number(
    int N, int k, double skew, std::vector<pair<int, uint32_t>>::iterator start,
    std::vector<pair<int, uint32_t>>::iterator end, FILE *output) {
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
    // printf("estimate (skew=%f,i=%i, pow(i, skew)=%f): %f\n", skew, i,
    //        pow((double)i, skew), estimate);
  }
  assert(i == k);
  auto index = estimates.begin() + k / 2;
  std::nth_element(estimates.begin(), index, estimates.end());
  double median = *index;
  // printf("Mean estimate of harmonic f (skew=%f): %f\n", skew,
  // total_estimate);

  return median;
}

double
estimate_harmonic_number(int N, int k, double skew,
                         std::vector<pair<int, uint32_t>>::iterator start,
                         std::vector<pair<int, uint32_t>>::iterator end) {
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
    // printf("estimate (skew=%f,i=%i, pow(i, skew)=%f): %f\n", skew, i,
    //        pow((double)i, skew), estimate);
  }
  assert(i == k);
  auto index = estimates.begin() + k / 2;
  std::nth_element(estimates.begin(), index, estimates.end());
  double median = *index;
  // printf("Mean estimate of harmonic f (skew=%f): %f\n", skew,
  // total_estimate);

  return median;
}

double
harmonic_number_variance(int N, int k, double skew,
                         std::vector<pair<int, uint32_t>>::iterator start,
                         std::vector<pair<int, uint32_t>>::iterator end) {
  assert(skew < 1.8);
  int i = 0;

  // This algorithm is potentially vulnerable to floating point cancellation.

  double inv_total = 1.0 / (double)N;
  double inv_k = 1.0 / (double)k;

  // double avr_sum_squares = 0.0;
  // double avr_sum = 0.0;

  std::vector<double> estimates;
  estimates.reserve(k);
  //  Loop in inverse order since largest is at the end
  for (auto it = --end; it >= start; --it) {
    i++;
    uint32_t count = it->second;
    double frequency = (double)count * inv_k;
    double val = frequency * pow((double)i, skew);
    double estimate = 1.0 / val;
    // avr_sum += inv_k * estimate;
    // avr_sum_squares += inv_k * estimate * estimate;
    estimates.push_back(estimate);
    //  printf("estimate (skew=%f,i=%i, pow(i, skew)=%f): %f\n", skew, i,
    //         pow((double)i, skew), estimate);
  }
  assert(i == k);
  double sum = 0.0;

  for (i = 0; i < k; i++) {
    sum += estimates[i];
  }

  double mean = sum / (double)k;

  // printf("skew = %f, mean = %f\n", skew, mean);

  double var = 0.0;
  for (i = 0; i < k; i++) {
    var += (estimates[i] - mean) * (estimates[i] - mean);
  }
  return var;

  // return avr_sum_squares - avr_sum;

  // auto index = estimates.begin() + k / 2;
  // std::nth_element(estimates.begin(), index, estimates.end());
  // double median = *index;
  // printf("Mean estimate of harmonic f (skew=%f): %f\n", skew,
  // total_estimate);
}

double cost_topk_skew(int N, int k, double skew_d,
                      std::vector<pair<int, uint32_t>>::iterator start,
                      std::vector<pair<int, uint32_t>>::iterator end) {
  return harmonic_number_variance(N, k, skew_d, start, end);
  // double cost = 0.0;
  // double harmonic_n = estimate_harmonic_number(N, k, skew_d, start, end);
  // printf("Skew: %f, harmonic_n: %f\n", skew_d, harmonic_n);
  // // double inv_harmonic_n = 1.0 / get_harmonic_number_or_calc(1 << 26,
  // skew); double inv_harmonic_n = 1.0 / harmonic_n; double inv_n = 1.0 /
  // (double)N;

  // int i = 0;
  // //  Loop in inverse order since largest is at the end
  // for (auto it = end; it != start; --it) {
  //   i++;
  //   uint32_t count = it->second;
  //   // printf("i: %d, count: %d\n", i, count);
  //   double expected = inv_harmonic_n / (pow((double)i, skew_d));
  //   double actual = inv_n * (double)count;
  //   // printf("Frequency for %i: %f (expected=%f), inv_harmonic_n=%f, pow(i,
  //   // skew)=%f\n", i, actual, expected, inv_harmonic_n, pow((double)i,
  //   skew));

  //   double diff = expected - actual;
  //   cost += diff * diff;
  // }

  // assert(i == k);

  // return cost / (float)k;
}

double cost_topk_skew_old(int N, int k, double skew, double harmonic_n,
                          std::vector<pair<int, uint32_t>>::iterator start,
                          std::vector<pair<int, uint32_t>>::iterator end) {
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

double
binary_search_estimate_skew(int N, int k,
                            std::vector<pair<int, uint32_t>>::iterator start,
                            std::vector<pair<int, uint32_t>>::iterator end) {
  const double skew_grad_delta = 0.0001;
  const int MAX_ITERS = 10;
  double bottom = 0.5;
  double top = 1.4;

  for (int i = 0; i < MAX_ITERS; i++) {
    double skew_f = (bottom + top) * 0.5;
    // int skew_i = floor(skew_f * 10.0 + 0.5);

    // TODO: replace with cost_topk_skew which will estimate the harmonic number
    // double harmonic = calculate_harmonic_number(1 << 26, skew_f);
    // double cost = cost_topk_skew_old(N, k, skew_f, harmonic, start, end);
    // double harmonic = estimate_harmonic_number(N, k, skew_f, start, end);
    // double cost = cost_topk_skew_old(N, k, skew_f, harmonic, start, end);
    // harmonic =
    //     estimate_harmonic_number(N, k, skew_f + skew_grad_delta, start, end);
    // double cost_plus_delta = cost_topk_skew_old(N, k, skew_f +
    // skew_grad_delta,
    //                                             harmonic, start, end);
    // harmonic = estimate_harmonic_number(N, k, skew_f + 0.1, start, end);
    // double cost_plus_big_delta =
    //     cost_topk_skew_old(N, k, skew_f + 0.1, harmonic, start, end);

    double cost = cost_topk_skew(N, k, skew_f, start, end);
    double cost_plus_delta =
        cost_topk_skew(N, k, skew_f + skew_grad_delta, start, end);

    // printf("for skew: %f, cost: %f, cost_plus_delta: %f, \n", skew_f, cost,
    //        cost_plus_delta);
    if (cost < cost_plus_delta) {
      // Cost is decreasing with a decreasing skew
      top = skew_f;
    } else if (cost > cost_plus_delta) {
      // Skew is decreasing with an increasing skew
      bottom = skew_f;
    } else {
      // printf(
      //     "skew cost did not change between %f and %f (iter = %i)
      //     (cost=%f)\n", skew_f, skew_f + skew_grad_delta, i, cost);
      return skew_f;
    }
  }

  return (bottom + top) * 0.5;
}
