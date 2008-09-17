#include <gtest/gtest.h>
#include "glop2d/Point2.h"

TEST(Point2Test, TestNormalize) {
  Point2 p;

  p = Point2(1.7, 3.9);
  p.Normalize();
  EXPECT_FLOAT_EQ(1.0, p.Norm());

  p = Point2(-1, 50);
  p.Normalize();
  EXPECT_FLOAT_EQ(1.0, p.Norm());

  p = Point2(-0.001, -0.0001);
  p.Normalize();
  EXPECT_FLOAT_EQ(1.0, p.Norm());

  p = Point2(100, 0);
  p.Normalize();
  EXPECT_FLOAT_EQ(1.0, p.Norm());
}

TEST(Point2Test, TestDotWithPerpendicularVector) {
  Point2 p(23.34, 56.767);
  Point2 perp = p;
  p.Rot90();
  EXPECT_FLOAT_EQ(0.0, Dot(p, perp));
}

TEST(Point2Test, TestRot90AgainstRotate90) {
  Point2 a(4, 5);
  Point2 b = a;
  a.Rotate(90);
  b.Rot90();
  EXPECT_FLOAT_EQ(a[0], b[0]);
  EXPECT_FLOAT_EQ(a[1], b[1]);
}
