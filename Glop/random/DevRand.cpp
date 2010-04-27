#include "DevRand.h"

DevRand::DevRand() {
  f_ = fopen("/dev/random", "r");
}

DevRand::~DevRand() {
  fclose(f_);
}

void DevRand::SerializeToString(string* data) const {
  // Shouldn't try to serialize this RNG
}

void DevRand::ParseFromString(const string& data) {
  // Shouldn't try to serialize this RNG
}

int DevRand::Rand() {
  int ret;
  fread(&ret, 1, 4, f_);
  return ret;
}
