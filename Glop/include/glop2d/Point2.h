// General purpose 2-dimensional vector utilities.

#ifndef GLOP_GLOP2D_POINT2_H__
#define GLOP_GLOP2D_POINT2_H__

// Includes
#include "../Base.h"

// Point2 class definition
class Point2 {
 public:
	// Constructors
	Point2() {data_[0] = data_[1] = 0;}
	Point2(float x, float y) {
    data_[0] = x;
    data_[1] = y;
  }
  const Point2 &operator=(const Point2 &rhs) {
    data_[0] = rhs.data_[0];
    data_[1] = rhs.data_[1];
    return *this;
  }
	Point2(const Point2 &rhs) {
		operator=(rhs);
  }
  
	// Accessors
  float operator[](int index) const {return data_[index];}
	float &operator[](int index) {return data_[index];}
	const float *GetData() const {return data_;}
	float *GetData() {return data_;}
	
	// Translation
	const Point2 &operator+=(const Point2 &rhs) {
		data_[0] += rhs.data_[0];
    data_[1] += rhs.data_[1];
    return *this;
  }
  const Point2 &operator-=(const Point2 &rhs) {
		data_[0] -= rhs.data_[0];
    data_[1] -= rhs.data_[1];
    return *this;
  }
	Point2 operator+(const Point2 &rhs) const {
		Point2 temp(*this); 
    return (temp+=rhs);
  }
	Point2 operator-(const Point2 &rhs) const {
		Point2 temp(*this);
    return (temp-=rhs);
  }
	
	// Scalar multiplication
	Point2 operator-() const {
    return Point2(-data_[0], -data_[1]);
  }
	const Point2 &operator*=(float scale) {
    data_[0] *= scale;
    data_[1] *= scale;
    return *this;
  }
	const Point2 &operator/=(float scale) {
    data_[0] /= scale;
    data_[1] /= scale;
    return *this;
  }
	Point2 operator*(float scale) const {
		Point2 temp(*this);
    return (temp*=scale);
  }
	Point2 operator/(float scale) const {
		Point2 temp(*this);
    return (temp/=scale);
  }

  // Utilities - see also non-class functions below for non-unary operations or for
  // versions of Rotate/Project/Normalize that return the result instead of modifying the point
  // directly.
  void Normalize() {operator/=(Norm());}
	void Rotate(float degrees);
	void Rotate(const Point2 &origin, float degrees) {
    operator-=(origin);
    Rotate(degrees);
    operator+=(origin);
  }
  inline void Project(const Point2 &axis);
  float Norm() const;

  // Comparators
  bool operator==(const Point2 &rhs) const {
    return IsEqual(data_[0], rhs.data_[0]) &&
           IsEqual(data_[1], rhs.data_[1]);
  }
	bool operator!=(const Point2 &rhs) const {
    return !operator==(rhs);
  }

 private:
	float data_[2];
};
inline Point2 operator*(float scale, const Point2 &rhs) {return rhs * scale;}
typedef Point2 Vec2;

// Point constants
const Point2 kOrigin2(0, 0);
const Vec2 kXAxis2(1, 0);
const Vec2 kYAxis2(0, 1);

// Point utilities
inline Point2 Normalize(Point2 x) {x.Normalize(); return x;}
inline float Cross(const Point2 &lhs, const Point2 &rhs) {  // Return the z-coordinate of the cross
	return lhs[0]*rhs[1] - lhs[1]*rhs[0];
}
inline float Dot(const Point2 &lhs, const Point2 &rhs) {
	return lhs[0]*rhs[0] + lhs[1]*rhs[1];
}
inline float Dist(const Point2 &lhs, const Point2 &rhs) {return (rhs - lhs).Norm();}
inline Point2 Rotate(Point2 x, float degrees) {
  x.Rotate(degrees);
  return x;
}
inline Point2 Rotate(Point2 x, const Point2 &origin, float degrees) {
  x.Rotate(origin, degrees);
  return x;
}
inline void Point2::Project(const Point2 &axis) {
  operator=(axis * (Dot(*this, axis) / Dot(axis, axis)));
}
inline Point2 Project(Point2 x, const Point2 &axis) {
  x.Project(axis);
  return x;
}

#endif // GLOP_GLOP2D_POINT2_H__
