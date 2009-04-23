#include "CMWC.h"
#include <stdlib.h>

CMWC::CMWC() {
  a_ = 1000000407;
  r_ = 16;
  rp2_ = 1;
  while (rp2_ < r_) {
    rp2_ <<= 1;
  }
  x_ = (unsigned int*)malloc(sizeof(unsigned int) * rp2_);
  for (int i = 0; i < r_; i++) {
    x_[i] = i;
  }
  pos_ = 0;
  c_ = 0;
  for (int i = 0; i < r_; i++) {
    Rand();
  }
}

CMWC::~CMWC() {
  free(x_);
}

void CMWC::SerializeToString(string* data) const {
  // TODO: Only need to send r_ values from x_, not rp2_.  Leaving it this way
  // for now because it's easier to make sure it's right, and r_ == rp2_ in the
  // usual case anyway.
  data->resize(sizeof(unsigned int) * (4 + rp2_));
  unsigned int* serialized = (unsigned int*)data->data();
  serialized[0] = r_;
  serialized[1] = a_;
  serialized[2] = pos_;
  serialized[3] = c_;
  for (int i = 0; i < rp2_; i++) {
    serialized[4 + i] = x_[i];
  }
}

void CMWC::ParseFromString(const string& data) {
  unsigned int* serialized = (unsigned int*)data.data();
  r_   = serialized[0];
  a_   = serialized[1];
  pos_ = serialized[2];
  c_   = serialized[3];
  rp2_ = 1;
  while (rp2_ < r_) {
    rp2_ <<= 1;
  }
  if (x_ != NULL) {
    free(x_);
  }
  x_ = (unsigned int*)malloc(sizeof(unsigned int) * rp2_);
  for (int i = 0; i < rp2_; i++) {
    x_[i] = serialized[4 + i];
  }
}


int32 CMWC::Rand() {
  int index = (pos_ - r_ + rp2_ - 1) & (rp2_ - 1);
  unsigned long long res =
      static_cast<unsigned long long>(a_) *
      static_cast<unsigned long long>(x_[index]) +
      static_cast<unsigned long long>(c_);
  unsigned int xn = 0x7fffffff - static_cast<unsigned int>(res);
  unsigned int cn = static_cast<unsigned int>(res >> 32);
  x_[pos_] = xn;
  c_ = cn;
  pos_ = (pos_ + 1) % rp2_;
//  return rand() - RAND_MAX/2;
  return xn;
}
