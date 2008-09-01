// Includes
#include "../../include/glop2d/Point2.h"
#include <cmath>

float Point2::Norm() const {
	return (float)sqrt(Dot(*this, *this));
}

void Point2::Rotate(float degrees) {
  float x = data_[0], y= data_[1];
	float c = (float)cos(degrees * kPi / 180);
	float s = (float)sin(degrees * kPi / 180);
  data_[0] = x*c + y*s;
  data_[1] = -x*s + y*c;
}
