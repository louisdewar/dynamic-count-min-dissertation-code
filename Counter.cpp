#include "Counter.hpp"
#include <assert.h>
#include "Defs.hpp"

PacketCounter::PacketCounter(int length) {
  this->buf = new int[length];

  // Probably unnecessary
  for (int i = 0; i < length; i++) {
    this->buf[i] = 0;
  }

  this->len = length;
}

int PacketCounter::increment(char *str) {
  int index = *(int*) str;

  int counter = ++this->buf[index];
  assert(index < this->len && "Index outside of range");

  return counter;
}

int PacketCounter::query(char *str) {
  int index = *(int*) str;
  assert(index < this->len && "Index outside of range");

  return this->buf[index];
}

int PacketCounter::query_index(int index) {
  assert(index < this->len && "Index outside of range");
  return this->buf[index];
}


void PacketCounter::reset() {
   this->buf = new int[this->len];
}
