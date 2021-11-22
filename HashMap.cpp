#include "HashMap.hpp"
#include "Defs.hpp"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Node::Node(char* str) {
  memcpy(&this->str, str, FT_SIZE);
  this->count = 1;
  this->next = NULL;
}

Node::~Node() {}

bool Node::equals(Node* other) {
  for (int i = 0; i < FT_SIZE; i++) {
    if(this->str[i] != other->str[i]) {
      return false;
    }
  }

  return true;
}

HashMapCounter::HashMapCounter(size_t capacity, uint seed) {
  this->cap = capacity;
  this->hasher = BOBHash(seed);
  this->table = new Node*[this->cap];
}

HashMapCounter::~HashMapCounter() {
  for (size_t i = 0; i < this->cap; i++) {
    Node* current = this->table[i];
    while(current != NULL) {
      Node* next = current->next;
      free(current);
      current = next;
    }
  }
  free(this->table);
}

void HashMapCounter::reset() {
  for (size_t i = 0; i < this->cap; i++) {
    Node* current = this->table[i];
    while(current != NULL) {
      current->count = 0;
      Node* next = current->next;
      current = next;
    }
  }
}

uint HashMapCounter::increment(char *str) {
  Node* node = new Node(str);

  uint index = this->hasher.run(str, FT_SIZE) % this->cap;

  Node* pointer = this->table[index];

  if (pointer == NULL) {
    // Insert into empty bucket
    this->table[index] = node;
    return 1;
  }

  while(pointer->next != NULL) {
    if(pointer->equals(node)) {
      pointer->count++;
      return pointer->count;
    }

    pointer = pointer->next;
  }

  // Insert into linked list
  pointer->next = node;

  return 1;
}

uint HashMapCounter::query(char *str) {
  Node* node = new Node(str);

  uint index = this->hasher.run(str, FT_SIZE) % this->cap;

  Node* pointer = this->table[index];

  if (pointer == NULL) {
    return 0;
  }

  while(pointer->next != NULL) {
    if(pointer->equals(node)) {
      return pointer->count;
    }

    pointer = pointer->next;
  }

  return 0;
}
