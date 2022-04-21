#pragma once

#include "Defs.hpp"
#include "xxhash.h"
#include <array>
#include <stdint.h>

using namespace std;
struct ArrayHasher {
  std::size_t operator()(const std::array<char, FT_SIZE> &a) const {
    XXH64_hash_t hash = XXH64(&a, FT_SIZE, 1010);
    return hash;
  }
};
