#include "Counter.hpp"
#include "Defs.hpp"
#include <assert.h>
#include <string.h>

PacketCounter::PacketCounter(int length) {
  this->buf = new int[length];

  // Probably unnecessary
  for (int i = 0; i < length; i++) {
    this->buf[i] = 0;
  }

  this->len = length;
}

PacketCounter::~PacketCounter() { delete[] this->buf; }

int PacketCounter::increment(char *str) {
  int index = *(int *)str;

  if (index >= this->len) {
    printf("Index: %d, len: %d\n", index, this->len);
  }
  assert(index < this->len && "Index outside of range");
  int counter = ++this->buf[index];

  return counter;
}

int PacketCounter::query(char *str) {
  int index = *(int *)str;
  assert(index < this->len && "Index outside of range");

  return this->buf[index];
}

int PacketCounter::query_index(int index) {
  assert(index < this->len && "Index outside of range");
  return this->buf[index];
}

void PacketCounter::reset() { memset(this->buf, 0, sizeof(int) * this->len); }

HashPacketCounter::HashPacketCounter(int length) {
  // this->map
}

HashPacketCounter::~HashPacketCounter() {
  // delete[] this->buf;
}

int HashPacketCounter::increment(char *str) {
  std::array<char, FT_SIZE> flow_id;
  memcpy(&flow_id, str, FT_SIZE);

  auto val = this->map.find(flow_id);

  if (val == this->map.end()) {
    this->map.insert(std::make_pair(flow_id, 0));
    val = this->map.find(flow_id);
  }

  int counter = val->second++;

  return counter;
}

int HashPacketCounter::query(char *str) {
  std::array<char, FT_SIZE> flow_id;
  memcpy(&flow_id, str, FT_SIZE);

  auto val = this->map.find(flow_id);

  return val->second;
}

// int HashPacketCounter::query_index(int index) {
//   // assert(index < this->len && "Index outside of range");
//   return this->map.find(const key_type &__x);
// }

// void HashPacketCounter::reset() {
//   memset(this->buf, 0, sizeof(int) * this->len);
// }
