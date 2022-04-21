// Code adapted from SALSA:
// https://github.com/SALSA-ICDE2021/SALSA/tree/main/Salsa

#pragma once

#include "ArrayHasher.hpp"
#include "Defs.hpp"
#include <assert.h>
#include <map>
#include <stdint.h>
#include <string.h>
#include <unordered_map>
#include <vector>

using namespace std;

// Adapted from SALSA (https://github.com/SALSA-ICDE2021/SALSA/tree/main/Salsa).
class TopK {
  unordered_map<std::array<char, FT_SIZE>, uint32_t, ArrayHasher> kvm;
  map<pair<uint32_t, std::array<char, FT_SIZE>>, std::array<char, FT_SIZE>>
      inverse_kvm;

  int k;

public:
  TopK(int k);

  vector<pair<std::array<char, FT_SIZE>, uint32_t>> items();

  void update(const char *packet, uint32_t value);
};
