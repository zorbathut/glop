#ifndef GLOP_RANDOM_RANDOM_H__
#define GLOP_RANDOM_RANDOM_H__

#include "../Base.h"

#include <string>
using namespace std;

class Random {
 public:
  virtual ~Random() {};
  int32 Int32();
  int64 Int64();
  float Range(float start, float end);
  virtual void SerializeToString(string* data) const = 0;
  virtual void ParseFromString(const string& data) = 0;

 protected:
  virtual int32 Rand() = 0;
};

#endif // GLOP_RANDOM_RANDOM_H__
