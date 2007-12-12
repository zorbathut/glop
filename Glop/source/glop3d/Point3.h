// General purpose 3-dimensional vector utilities.
//
// Point3/Vec3: A 3-dimensional point/vector.
// Viewpoint: A position and orientation (forward and up vectors).

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
inline Point3 operator*(float scale, const Point3 &rhs) {return rhs * scale;}
typedef Point3 Vec3;

// Point constants
const Point3 kOrigin(0, 0, 0);
const Vec3 kXAxis(1, 0, 0);
const Vec3 kYAxis(0, 1, 0);
const Vec3 kZAxis(0, 0, 1);

// Viewpoint class definition. This is a position plus a view orientation.
class Viewpoint {
 public:
  // Constructors. In general, one constructs a Viewpoint from a forward vector and a requested
  // up vector (which is implicitly (0, 1, 0) if nothing is specified). It is guaranteed that the
  // actual up vector will be in the same plane as the forward vector and the requested up vector.
  // Thus Viewpoint(x) creates a Viewpoint with no roll facing towards x.
  Viewpoint()
  : position_(),
    forward_vector_(0,0,1),
    up_vector_(0,1,0),
    right_vector_(1,0,0) {}
  Viewpoint(const Point3 &position)
  : position_(position),
    forward_vector_(0,0,1),
    up_vector_(0,1,0),
    right_vector_(1,0,0) {}
  Viewpoint(const Point3 &position, const Vec3 &forward_vector, const Vec3 &up_vector)
  : position_(position) {
    SetDirection(forward_vector, up_vector);
  }
  Viewpoint(const Viewpoint &rhs)
  : position_(rhs.position_),
    forward_vector_(rhs.forward_vector_),
    up_vector_(rhs.up_vector_),
    right_vector_(rhs.right_vector_) {}
  void operator=(const Viewpoint &rhs) {
    position_ = rhs.position_;
    forward_vector_ = rhs.forward_vector_;
    up_vector_ = rhs.up_vector_;
    right_vector_ = rhs.right_vector_;
  }

	// Accessors
	const Point3 &position() const {return position_;}
	const Vec3 &forwards() const {return forward_vector_;}
	const Vec3 &up() const {return up_vector_;}
	const Vec3 &right() const {return right_vector_;}

	// Mutators. SetDirection works in the same way as the constructor (see above).
  void SetPosition(const Vec3 &position) {position_ = position;}
	void SetDirection(const Vec3 &forward_vector) {SetDirection(forward_vector, up_vector_);}
	void SetDirection(const Vec3 &forward_vector, const Vec3 &up_vector);
	void Translate(const Point3 &translation) {SetPosition(position_ + translation);}
	void Rotate(const Vec3 &axis, float degrees);
	void Rotate(const Point3 &center, const Vec3 &axis, float degrees) {
		position_ -= center;
    Rotate(axis, degrees);
    position_ += center;
  }

  // Coordinate transformations. Each Viewpoint can be thought of specifying its own coordinate
  // system. Here, we provide utilities for switching between that coordinate system and the
  // "global" one (i.e., the normal, base coordinate system).
	Point3 LocalToGlobal(const Point3 &p) const;
	Point3 GlobalToLocal(const Point3 &p) const;
	Viewpoint LocalToGlobal(const Viewpoint &vp) const;
	Viewpoint GlobalToLocal(const Viewpoint &vp) const;

  // Returns LocalToGlobal in the form of an OpenGl matrix. Thus, an object positioned at this
  // ViewPoint can be rendered as follows:
  // float m[16]; FillTransformationMatrix(m); glMultMatrixf(m); Render();
  void FillTransformationMatrix(float *matrix) const;

  // Comparators
  bool operator==(const Viewpoint &rhs) const {
    return position_ == rhs.position_ &&
           forward_vector_ == rhs.forward_vector_ &&
           up_vector_ == rhs.up_vector_ &&
           right_vector_ == rhs.right_vector_;
  }
  bool operator!=(const Viewpoint &rhs) const {
    return !operator==(rhs);
  }

 private:
  // Private constructor - used by the coordinate transformations. This is more efficient than
  // constructing a blank Viewpoint and filling its data manually.
  Viewpoint(const Point3 &position, const Vec3 &forward_vector, const Vec3 &up_vector,
            const Vec3 &right_vector)
  : position_(position),
    forward_vector_(forward_vector),
    up_vector_(up_vector),
    right_vector_(right_vector) {}

	Point3 position_;
  Vec3 forward_vector_, up_vector_, right_vector_;
};

#endif // GLOP_GLOP3D_POINT3_H__
