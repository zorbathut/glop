// General purpose 2-dimensional vector utilities.
//
// Point3/Vec3: A 2-dimensional point/vector.
// Viewpoint: A position and orientation (forward and up vectors).

#ifndef GLOP_GLOP3D_POINT3_H__
#define GLOP_GLOP3D_POINT3_H__

// Includes
#include "../Base.h"

// Point3 class definition
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

  // Vector mutators
  void Normalize() {operator/=(GetNorm());}
	void Rotate(float degrees);
	void Rotate(const Point2 &origin, float degrees) {
    operator-=(origin);
    Rotate(degrees);
    operator+=(origin);
  }
  void Project(const Point2 &axis) {
    operator=(axis * (Dot(axis) / axis.Dot(axis)));
  }

  // Vector utilities. For rotation, angles are specified in degrees counter-clockwise.
  // GetSide returns which side of a line we're on (-1 or 1, or 0 if it is on the segment).
	float Dot(const Point2 &rhs) const {
		return data_[0]*rhs.data_[0] + data_[1]*rhs.data_[1];
  }
  int GetSide(const Point2 &u, const Point2 &v) {
    Point2 a = (*this - u).GetNormal(), b = (v - u).GetNormal();
    float temp = a[1] * b[0] - a[0] * b[1];
    if (IsEqual(temp, 0))
      return 0;
    return (temp < 0? -1 : 1);
  }
  float GetNorm() const;
  Point2 GetNormal() const {return *this / GetNorm();}
  float GetDist(const Point2 &rhs) const {return (*this - rhs).GetNorm();}
  Point2 GetRotation(float degrees) const {
    Point2 temp(*this);
    temp.Rotate(degrees);
    return temp;
  }
	Point2 GetRotation(const Point2 &origin, float degrees) const {
    Point2 temp(*this);
    temp.Rotate(origin, degrees);
    return temp;
  }
  Point2 GetProjection(const Point2 &axis) const {
    Point2 temp(*this);
    temp.Project(axis);
    return temp;
  }

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

#endif // GLOP_GLOP3D_POINT3_H__
