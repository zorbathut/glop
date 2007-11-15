// Includes
#include "Point3.h"
#include <cmath>

// Distance
float Point3::GetNorm() const {
	return (float)sqrt(Dot(*this));
}

// Rotation using the formula:
//
// p' = p*cos(theta) - (p x axis)*sin(theta) + axis*(p . axis)*(1 - cos(theta)).
//
// Note that scalar*scalar*vector is evaluated fastest by multiplying scalars first.
void Point3::Rotate(const Point3 &axis, float degrees) {
	float c = (float)cos(degrees * kPi / 180);
	float s = (float)sin(degrees * kPi / 180);
	
	// Note scalar*scalar*vector is evaluated fastest if the scalars are combined
	// first.
  *this = (*this)*c - Cross(axis)*s + axis*(Dot(axis)*(1-c));
}
