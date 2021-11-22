#pragma once

#include "BobHash.hpp"
#include "Defs.hpp"

typedef unsigned int uint;

class Node {
  public:
    Node(char* str);
    ~Node();

    bool equals(Node* other);

    uint count;
    Node* next;
  private:
    char str[FT_SIZE];
};

class HashMapCounter {
public:
  HashMapCounter(size_t capacity, uint seed);
  ~HashMapCounter();

  // Expects a string of length FT_SIZE
  // Either inserts a new string with the value 1 or adds 1 to the existing value
  // and returns the new count
  uint increment(char* str);

  // Returns the number of times str has been incremented
  uint query(char* str);

  // Reset statistics
  void reset();
private:
  BOBHash hasher;
  size_t cap;
  Node** table;
};
