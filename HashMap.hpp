#pragma once

class HashMap {

public:
  HashMap();
  ~HashMap();

  void initialize(uint primeNum);
  uint run(const char *str, uint len);

  uint primeNum;

private:
};
