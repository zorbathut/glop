#include "Random.h"

int32 Random::Int32() {
  return Rand();
}

int64 Random::Int64() {
  return (static_cast<uint64>(Rand()) << 32) | static_cast<uint64>(Rand());
}

float Random::Range(float start, float end) {
  return static_cast<float>(Rand()&0x7fffffff) / ((0x7fffffff)/(end-start)) + start;
}

