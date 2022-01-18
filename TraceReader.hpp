#pragma once

#include <fstream>
#include <iostream>

class ZipfReader {
public:
  ZipfReader(char *path);
  ~ZipfReader();

  // `dest` must be at least FT_SIZE bytes long.
  int read_next_packet(char *dest);

private:
  std::filebuf *buffer;
  std::ifstream ifs;
};
