#pragma once

#include <array>
#include <math.h>
#include <stdio.h>

const int SKEW_START = 6;
// End is inclusive
const int SKEW_END = 13;
const int SKEW_COUNT = SKEW_END - SKEW_START + 1;
const long HARMONIC_TABLE_DOMAIN = 1 << 26;

constexpr double calculate_harmonic_number(long n, double skew) {
  double acc = 0.0;
  for (long i = 1; i <= n; i++) {
    acc += 1.0 / pow((double)i, skew);
  }

  return acc;
}

double calculate_harmonic_number_debug(long n, double skew) {
  printf("n: %lu, skew: %f\n", n, skew);
  double acc = 0.0;
  for (int i = 1; i < n; i++) {
    acc += 1.0 / pow((double)i, skew);

    if (i < 100) {
      printf("i: %d, acc: %f\n", i, acc);
    }
  }

  return acc;
}

constexpr std::array<double, SKEW_COUNT> build_harmonic_number_table() {
  std::array<double, SKEW_COUNT> table{};
  int i = 0;
  for (int skew = SKEW_START; skew <= SKEW_END; skew++) {
    double harmonic_n =
        calculate_harmonic_number(HARMONIC_TABLE_DOMAIN, 0.1 * (double)skew);
    table[i] = harmonic_n;
    i++;
  }

  return table;
}

const std::array<double, SKEW_COUNT> CONST_HARMONIC_TABLE =
    build_harmonic_number_table();

double get_harmonic_number_or_calc(long n, int skew) {
  double skew_d = 0.1 * (double)skew;

  if (skew >= SKEW_START && skew <= SKEW_END && n == HARMONIC_TABLE_DOMAIN) {
    int table_i = skew - SKEW_START;
    printf("using cached harmonic number\n");
    return CONST_HARMONIC_TABLE[table_i];
  } else {
    printf("calculating harmonic number\n");
    return calculate_harmonic_number(n, skew_d);
  }
}

void print_cached_skews() {
  for (int i = 0; i < SKEW_COUNT; i++) {
    double skew = 0.1 * (double)(SKEW_START + i);
    double harmonic_n = CONST_HARMONIC_TABLE[i];
    printf("For skew %f, cached harmonic number: %f\n", skew, harmonic_n);
  }
}

// class ZipTable {
// private:
//   int start;
//   int end;
//
// public:
// }
