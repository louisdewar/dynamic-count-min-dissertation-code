#pragma once

#include "BobHash.hpp"
#include "Defs.hpp"
#include "xxhash.h"
#include <unordered_map>

typedef unsigned int uint;

// This uses the fact that in the generated data the first 4 bytes is the unique
// identifier. The domain is 0-length.
class PacketCounter {
public:
  PacketCounter(int len);
  ~PacketCounter();

  // Only uses the first 4 bytes
  int increment(char *str);
  int query(char *str);
  int query_index(int index);

  void reset();

private:
  int len;
  int *buf;
};

struct ArrayHasher {
  std::size_t operator()(const std::array<int, FT_SIZE> &a) const {
    XXH64_hash_t hash = XXH64(&a, FT_SIZE, 1010);
    return hash;
    // std::size_t h = 0;

    // for (auto e : a) {
    //   h ^= std::hash<int>{}(e) + 0x9e3779b9 + (h << 6) + (h >> 2);
    // }
    // return h;
  }
};

class HashPacketCounter {
public:
  HashPacketCounter(int len);
  ~HashPacketCounter();

  // Only uses the first 4 bytes
  int increment(char *str);
  int query(char *str);
  int query_index(int index);

  void reset();

private:
  std::unordered_map<std::array<int, FT_SIZE>, int, ArrayHasher> map;
};

