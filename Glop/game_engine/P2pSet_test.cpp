#include <gtest/gtest.h>
#include "P2pSet.h"
#include <utility>
using namespace std;

void SerializeToString(const int &t, string *data) {
  data->resize(4);
  ((int*)data->data())[0] = t;
}

void ParseFromString(const string &data, int *t) {
  *t = ((int*)data.data())[0];
}

TEST(P2pSetTest, TestSerializePairOfInts) {
  pair<int, int> input = make_pair(2,3);
  string s;
  SerializeToString(input, &s);

  pair<int, int> output;
  ParseFromString(s, &output);
  EXPECT_EQ(2, output.first);
  EXPECT_EQ(3, output.second);
}

struct Thing {
  Thing(int _v1, int _v2): v1(_v1), v2(_v2) {}
  Thing() {}
  int v1;
  int v2;
  void SerializeToString(string *s) const {
    s->resize(8);
    ((int*)s->data())[0] = v1;
    ((int*)s->data())[1] = v2;
  }
  void ParseFromString(const string &s) {
    v1 = ((int*)s.data())[0];
    v2 = ((int*)s.data())[1];
  }
};

bool operator == (const Thing& a, const Thing& b) {
  return a.v1 == b.v1 && a.v2 == b.v2;
}

TEST(P2pSetTest, TestSerializeP2pSet) {
  P2pSet<Thing> ps1;
  string s;
  ps1.push_back(P2pSetId(1,2),Thing(3,4));
  ps1.push_back(P2pSetId(1,4),Thing(2,3));
  ps1.push_back(P2pSetId(3,1),Thing(3,5));
  ps1.push_back(P2pSetId(4,2),Thing(1,8));
  ps1.push_back(P2pSetId(5,6),Thing(6,7));
  ps1.SerializeToString(&s);

  P2pSet<Thing> ps2;
  ps2.ParseFromString(s);
  P2pSet<Thing>::iterator it1;
  P2pSet<Thing>::iterator it2;
  for (it1 = ps1.begin(), it2 = ps2.begin(); it1 != ps1.end() && it2 != ps2.end(); it1++, it2++) {
    EXPECT_EQ(ListId(it1).value(), ListId(it2).value());
    EXPECT_TRUE(it1.id() == it2.id());
    EXPECT_TRUE(*it1 == *it2);
  }
}
