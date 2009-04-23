#ifndef GLOP_RANDOM_CMWC_H__
#define GLOP_RANDOM_CMWC_H__

#include "Random.h"

#include <string>
using namespace std;

class CMWC : public Random {
 public:
  CMWC();
  virtual ~CMWC();
  virtual void SerializeToString(string* data) const;
  virtual void ParseFromString(const string& data);

 protected:
  virtual int32 Rand();

 private:
  unsigned int r_;
  unsigned int rp2_;  // the smallest power of two greater than or equal to r_.
  unsigned int* x_;
  unsigned int a_;
  unsigned int pos_;
  unsigned int c_;
};

#endif // GLOP_RANDOM_CMWC_H__
