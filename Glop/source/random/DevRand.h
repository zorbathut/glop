#ifndef GLOP_RANDOM_DEVRAND_H__
#define GLOP_RANDOM_DEVRAND_H__

#include "Random.h"
#include <stdio.h>

class DevRand : public Random {
 public:
  DevRand();
  virtual ~DevRand();
  virtual void SerializeToString(string* data) const;
  virtual void ParseFromString(const string& data);

 protected:
  virtual int32 Rand();

 private:
  FILE* f_;
};

#endif // GLOP_RANDOM_DEVRAND_H__
