// Includes
#include "../../include/glop3d/Point3.h"
#include <cmath>

// Point3
// ======

float Point3::GetNorm() const {
	return (float)sqrt(Dot(*this));
}

// Rotation using the formula:
//   p' = p*cos(theta) - (p x axis)*sin(theta) + axis*(p . axis)*(1 - cos(theta)).
// Note that scalar*scalar*vector is evaluated fastest by multiplying scalars first.
void Point3::Rotate(const Point3 &axis, float degrees) {
  Vec3 norm_ax = axis.GetNormal();
	float c = (float)cos(degrees * kPi / 180);
	float s = (float)sin(degrees * kPi / 180);
  *this = (*this)*c - Cross(norm_ax)*s + norm_ax*(Dot(norm_ax)*(1-c));
}

// Viewpoint
// =========

void Viewpoint::SetDirection(const Point3 &forward_vector, const Point3 &up_vector) {
  forward_vector_ = forward_vector.GetNormal();
	right_vector_ = up_vector.Cross(forward_vector_).GetNormal();
	up_vector_ = forward_vector_.Cross(right_vector_);
}

// We do not use Point3::Rotate so that we only need to compute the cosine and sine once.
void Viewpoint::Rotate(const Vec3 &axis, float degrees) {
  Vec3 norm_ax = axis.GetNormal();
	float c = (float)cos(degrees * kPi / 180);
	float s = (float)sin(degrees * kPi / 180);
  forward_vector_ = forward_vector_*c - forward_vector_.Cross(norm_ax)*s +
                    norm_ax*(forward_vector_.Dot(norm_ax)*(1-c));
  up_vector_ = up_vector_*c - up_vector_.Cross(norm_ax)*s +
               norm_ax*(up_vector_.Dot(norm_ax)*(1-c));
  right_vector_ = right_vector_*c - right_vector_.Cross(norm_ax)*s +
                  norm_ax*(right_vector_.Dot(norm_ax)*(1-c));
}

Point3 Viewpoint::LocalToGlobal(const Point3 &p) const {
	return p[0]*right_vector_ + p[1]*up_vector_ + p[2]*forward_vector_;
}
Point3 Viewpoint::GlobalToLocal(const Point3 &p) const {
	return Point3(p.Dot(right_vector_), p.Dot(up_vector_), p.Dot(forward_vector_));
}
Viewpoint Viewpoint::LocalToGlobal(const Viewpoint &vp) const {
    return Viewpoint(LocalToGlobal(vp.position_),
                     LocalToGlobal(vp.forward_vector_),
                     LocalToGlobal(vp.up_vector_),
                     LocalToGlobal(vp.right_vector_));
}
Viewpoint Viewpoint::GlobalToLocal(const Viewpoint &vp) const {
    return Viewpoint(GlobalToLocal(vp.position_),
                     GlobalToLocal(vp.forward_vector_),
                     GlobalToLocal(vp.up_vector_),
                     GlobalToLocal(vp.right_vector_));
}

void Viewpoint::FillTransformationMatrix(float *matrix) const {
  matrix[ 0] = right_vector_[0];
  matrix[ 1] = right_vector_[1];
  matrix[ 2] = right_vector_[2];
  matrix[ 3] = 0;
  matrix[ 4] = up_vector_[0];
  matrix[ 5] = up_vector_[1];
  matrix[ 6] = up_vector_[2];
  matrix[ 7] = 0;
  matrix[ 8] = forward_vector_[0];
  matrix[ 9] = forward_vector_[1];
  matrix[10] = forward_vector_[2];
  matrix[11] = 0;
  matrix[12] = position_[0];
  matrix[13] = position_[1];
  matrix[14] = position_[2];
  matrix[15] = 1;
}
