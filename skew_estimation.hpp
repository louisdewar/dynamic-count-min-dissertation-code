#pragma once

#include "Defs.hpp"
#include <algorithm>
#include <array>
#include <assert.h>
#include <cstdint>
#include <math.h>
#include <stdio.h>
#include <utility>
#include <vector>

using namespace std;

double debuging_estimate_harmonic_number(
    int N, int k, double skew,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator start,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator end,
    FILE *output);
double estimate_harmonic_number(
    int N, int k, double skew,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator start,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator end);
double harmonic_number_variance(
    int N, int k, double skew,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator start,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator end);
double cost_topk_skew(
    int N, int k, double skew_d,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator start,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator end);
double cost_topk_skew_old(
    int N, int k, double skew, double harmonic_n,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator start,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator end);
double binary_search_estimate_skew(
    int N, int k,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator start,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator end);

double small_set_estimate_skew(
    int N, int k,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator start,
    std::vector<pair<std::array<char, FT_SIZE>, uint32_t>>::iterator end);
