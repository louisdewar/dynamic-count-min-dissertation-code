#pragma once

#include "ArrayHasher.hpp"
#include "BobHash.hpp"
#include "Defs.hpp"
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

// struct ArrayHasher {
//   std::size_t operator()(const std::array<char, FT_SIZE> &a) const {
//     XXH64_hash_t hash = XXH64(&a, FT_SIZE, 1010);
//     return hash;
//   }
// };

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
  std::unordered_map<std::array<char, FT_SIZE>, int, ArrayHasher> map;
};
