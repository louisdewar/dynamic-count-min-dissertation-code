#include "TraceReader.hpp"
#include "Defs.hpp"

ZipfReader::ZipfReader(char *path) {
  this->ifs = std::ifstream(path);

  if (!ifs) {
    std::string msg = "Failed to open zipf file for reading --";
    msg += path;
    msg += "--";
    throw std::runtime_error(msg);
  }

  char dest[FT_SIZE] = {};
  this->buffer = ifs.rdbuf();
  this->buffer->sgetn((char *)&dest, FT_SIZE);
}

ZipfReader::~ZipfReader() { this->buffer->close(); }

int ZipfReader::read_next_packet(char *dest) {
  return this->buffer->sgetn(dest, FT_SIZE);
}
