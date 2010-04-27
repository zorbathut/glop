#include <gtest/gtest.h>
#include "MovingWindow.h"

TEST(MovingWindowText, TestEverythingIsDefaultConstructed) {
  // We do this twice, once to set a bunch of memory to all 1's, then another time to test if that
  // same memory gets default constructed to 0's.  I'm not sure if we're guaranteed that we'll use
  // the same memory the second time around, but it seems to work.
  MovingWindow<int> mv(10,5);
  for (int i = 0; i < 2; i++) {
    mv = MovingWindow<int>(10,5);
    ASSERT_EQ(5, mv.GetFirstIndex());
    ASSERT_EQ(14, mv.GetLastIndex());
    for (int i = mv.GetFirstIndex(); i <= mv.GetLastIndex(); i++) {
      EXPECT_EQ(0, mv[i]);
      mv[i] = 1;
    }
  }
}

// TODO: Make a reset-and-clear method


TEST(MovingWindowText, TestSetWithoutMoving) {
  MovingWindow<int> mv(10,5);
  ASSERT_EQ(5, mv.GetFirstIndex());
  ASSERT_EQ(14, mv.GetLastIndex());
  for (int i = mv.GetFirstIndex(); i <= mv.GetLastIndex(); i++) {
    mv[i] = i;
  }
  for (int i = mv.GetFirstIndex(); i <= mv.GetLastIndex(); i++) {
    EXPECT_EQ(i, mv[i]);
  }
}

TEST(MovingWindowText, TestSetWithMoving) {
  MovingWindow<int> mv(10,5);
  ASSERT_EQ(5, mv.GetFirstIndex());
  ASSERT_EQ(14, mv.GetLastIndex());
  for (int i = mv.GetFirstIndex(); i <= mv.GetLastIndex(); i++) {
    mv[i] = i;
  }
  mv.Advance();
  mv[mv.GetLastIndex()] = mv.GetLastIndex();
  for (int i = mv.GetFirstIndex(); i <= mv.GetLastIndex(); i++) {
    EXPECT_EQ(i, mv[i]);
  }
}

TEST(MovingWindowText, TestMovingForALongTime) {
  MovingWindow<int> mv(10,5);
  ASSERT_EQ(5, mv.GetFirstIndex());
  ASSERT_EQ(14, mv.GetLastIndex());
  for (int i = mv.GetFirstIndex(); i <= mv.GetLastIndex(); i++) {
    mv[i] = i;
  }
  for (int i = 0; i < 500; i++) {
    mv.Advance();
    EXPECT_EQ(0, mv[mv.GetLastIndex()]);
    mv[mv.GetLastIndex()] = mv.GetLastIndex();
    for (int j = mv.GetFirstIndex(); j <= mv.GetLastIndex(); j++) {
      EXPECT_EQ(j, mv[j]);
    }
  }
}
