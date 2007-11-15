// A general purpose 3-dimensional point class. The point is stored as an array of floats for easy
// use with OpenGl system calls.

#ifndef GLOP_GLOP3D_POINT3_H__
#define GLOP_GLOP3D_POINT3_H__

// Includes
#include "../Base.h"

// Point3 class definition
class Point3 {
public:
	// Constructors
	Point3() {data_[0] = data_[1] = data_[2] = 0;}
	Point3(float x, float y, float z) {
    data_[0] = x;
    data_[1] = y;
    data_[2] = z;
  }
  const Point3 &operator=(const Point3 &rhs) {
    data_[0] = rhs.data_[0];
    data_[1] = rhs.data_[1];
    data_[2] = rhs.data_[2];
    return *this;
  }
	Point3(const Point3 &rhs) {
		operator=(rhs);
  }
  
	// Accessors
  float operator[](int index) const {return data_[index];}
	float &operator[](int index) {return data_[index];}
	const float *getData() const {return data_;}
	float *getData() {return data_;}
	
	// Translation
	const Point3 &operator+=(const Point3 &rhs) {
		data_[0] += rhs.data_[0];
    data_[1] += rhs.data_[1];
    data_[2] += rhs.data_[2];
    return *this;
  }
  const Point3 &operator-=(const Point3 &rhs) {
		data_[0] -= rhs.data_[0];
    data_[1] -= rhs.data_[1];
    data_[2] -= rhs.data_[2];
    return *this;
  }
	Point3 operator+(const Point3 &rhs) const {
		Point3 temp(*this); 
    return (temp+=rhs);
  }
	Point3 operator-(const Point3 &rhs) const {
		Point3 temp(*this);
    return (temp-=rhs);
  }
	
	// Scalar multiplication
	Point3 operator-() const {
    return Point3(-data_[0], -data_[1], -data_[2]);
  }
	const Point3 &operator*=(float scale) {
    data_[0] *= scale;
    data_[1] *= scale;
    data_[2] *= scale;
    return *this;
  }
	const Point3 &operator/=(float scale) {
    data_[0] /= scale;
    data_[1] /= scale;
    data_[2] /= scale;
    return *this;
  }
	Point3 operator*(float scale) const {
		Point3 temp(*this);
    return (temp*=scale);
  }
	Point3 operator/(float scale) const {
		Point3 temp(*this);
    return (temp/=scale);
  }

  // Vector mutators
  void Normalize() {operator/=(GetNorm());}
	void Rotate(const Point3 &axis, float degrees);
	void Rotate(const Point3 &origin, const Point3 &axis, float degrees) {
    operator-=(origin);
    Rotate(axis, degrees);
    operator+=(origin);
  }
  void Project(const Point3 &axis) {
    operator=(axis * (Dot(axis) / axis.Dot(axis)));
  }

  // Vector utilities. For rotation, angles are specified in degrees counter-clockwise.
	Point3 Cross(const Point3 &rhs) const {
		return Point3(data_[1]*rhs.data_[2] - data_[2]*rhs.data_[1],
					        data_[2]*rhs.data_[0] - data_[0]*rhs.data_[2],
					        data_[0]*rhs.data_[1] - data_[1]*rhs.data_[0]);
  }
	float Dot(const Point3 &rhs) const {
		return data_[0]*rhs.data_[0] + data_[1]*rhs.data_[1] + data_[2]*rhs.data_[2];
  }
  float GetNorm() const;
  Point3 GetNormal() const {return *this / GetNorm();}
  float GetDist(const Point3 &rhs) const {return (*this - rhs).GetNorm();}
  Point3 GetRotation(const Point3 &axis, float degrees) const {
    Point3 temp(*this);
    temp.Rotate(axis, degrees);
    return temp;
  }
	Point3 GetRotation(const Point3 &origin, const Point3 &axis, float degrees) const {
    Point3 temp(*this);
    temp.Rotate(origin, axis, degrees);
    return temp;
  }
  Point3 GetProjection(const Point3 &axis) const {
    Point3 temp(*this);
    temp.Project(axis);
    return temp;
  }

  // Comparators
  bool operator==(const Point3 &rhs) const {
    return IsEqual(data_[0], rhs.data_[0]) &&
           IsEqual(data_[1], rhs.data_[1]) &&
           IsEqual(data_[2], rhs.data_[2]);
  }
	bool operator!=(const Point3 &rhs) const {
    return !operator==(rhs);
  }

private:
	float data_[3];
};

// Type definitions
typedef Point3 Vec3;

// Global functions
inline Point3 operator*(float scale, const Point3 &rhs) {return rhs * scale;}

#endif // GLOP_GLOP3D_POINT3_H__
