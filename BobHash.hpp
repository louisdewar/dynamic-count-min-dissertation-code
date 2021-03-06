// Code adapted from SALSA:
// https://github.com/SALSA-ICDE2021/SALSA/tree/main/Salsa

// Adapted from:
// https://github.com/Gavindeed/HeavyGuardian/tree/master/flow_size

#pragma once

#ifndef BOB_HASH_H
#define BOB_HASH_H

#include <stdio.h>

typedef unsigned int uint;

class BOBHash {

public:
  BOBHash();
  ~BOBHash();
  BOBHash(uint primeNum);
  void initialize(uint primeNum);
  uint run(const char *str, uint len);

  uint primeNum;

private:
};

#endif // !BOB_HASH_H
