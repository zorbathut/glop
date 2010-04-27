#include <gtest/gtest.h>

#include "CMWC.h"
#include "DevRand.h"

#include <stdio.h>

#include <vector>
using namespace std;

TEST(RandomTest, TestSerializing) {
  CMWC r;
  for (int i = 0; i < 10000; i++) {
    r.Int32();
  }
  string serialized;
  r.SerializeToString(&serialized);
  CMWC r2;
  r2.ParseFromString(serialized);
  for (int i = 0; i < 1000; i++) {
    EXPECT_EQ(r.Int32(), r2.Int32());
  }
}

/*
TEST(RandomTest, WriteToFile) {
  CMWC r;
  FILE* f = fopen("cmwc.rand","w");
  for (int i = 0; i < 3000000; i++) {
    int32 d = r.Int32();
    fwrite(&x, 1, 4, f);
  }
  fclose(f);
}
*/
