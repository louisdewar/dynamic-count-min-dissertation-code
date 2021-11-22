#include "Counter.hpp"
#include "Defs.hpp"

PacketCounter::PacketCounter(int length) {
  this->buf = new int[length];
  this->len = length;
}

int PacketCounter::increment(char *str) {
  int index = *(int*) str;

  int counter = ++this->buf[index];

  return counter;
}

int PacketCounter::query(char *str) {
  int index = *(int*) str;

  return this->buf[index];
}

void PacketCounter::reset() {
   this->buf = new int[this->len];
}
