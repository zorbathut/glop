// Includes
#include "ViewPoint.h"
#include "../GlopWindow.h"
#include "../OpenGl.h"
#include <cmath>

extern GlopWindow* gWindow;

// ViewPoint
// =========

// Makes the object point in the given direction - the up vector is chosen to be on the 
// same plane as the given up vector and the given forward vector, perpendicular to
// the given forward vector.
void ViewPoint::SetDirection(const Point3 &forward_vector, const Point3 &up_vector) {
  forward_vector_ = forward_vector.GetNormal();
	right_vector_ = up_vector.Cross(forward_vector).GetNormal();
	up_vector_ = forward_vector.Cross(right_vector_);
}

// Does a rotation. We do not use Point3::Rotate so that we only need to compute the cosine and
// sine once.
void ViewPoint::Rotate(const Vec3 &axis, float degrees) {
	float c = (float)cos(degrees * kPi / 180);
	float s = (float)sin(degrees * kPi / 180);
  forward_vector_ = forward_vector_*c - forward_vector_.Cross(axis)*s +
                    axis*(forward_vector_.Dot(axis)*(1-c));
  up_vector_ = up_vector_*c - up_vector_.Cross(axis)*s +
               axis*(up_vector_.Dot(axis)*(1-c));
  right_vector_ = right_vector_*c - right_vector_.Cross(axis)*s +
                  axis*(right_vector_.Dot(axis)*(1-c));
}

// Coordinate transformations. See ViewPoint.h.
Point3 ViewPoint::LocalToGlobal(const Point3 &p) const {
	return p[0]*right_vector_ + p[1]*up_vector_ + p[2]*forward_vector_;
}
Point3 ViewPoint::GlobalToLocal(const Point3 &p) const {
	return Point3(p.Dot(right_vector_), p.Dot(up_vector_), p.Dot(forward_vector_));
}
ViewPoint ViewPoint::LocalToGlobal(const ViewPoint &vp) const {
    return ViewPoint(LocalToGlobal(vp.position_),
                     LocalToGlobal(vp.forward_vector_),
                     LocalToGlobal(vp.up_vector_),
                     LocalToGlobal(vp.right_vector_));
}
ViewPoint ViewPoint::GlobalToLocal(const ViewPoint &vp) const {
    return ViewPoint(GlobalToLocal(vp.position_),
                     GlobalToLocal(vp.forward_vector_),
                     GlobalToLocal(vp.up_vector_),
                     GlobalToLocal(vp.right_vector_));
}

// Assuming matrix is a float array of size 16, this fills it with the modelview transformation
// required to transform local coordinates in this viewPoint to global coordinates.
// TODO: Provide example usage.
void ViewPoint::FillTransformationMatrix(float *matrix) const {
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

// Camera
// ======

// See ViewPoint.h.
bool Camera::IsInFrustum(const Point3 &center, float radius) const {
	Point3 local_center = center - GetPosition();
	float z_dist = local_center.Dot(front_normal_);
	if (z_dist+radius < near_plane_ || z_dist-radius > far_plane_ ||
      (is_fog_enabled_ && z_dist-radius > fog_end_))
    return false;
  if (local_center.Dot(top_normal_) < -radius) return false;
	if (local_center.Dot(bottom_normal_) < -radius) return false;
	if (local_center.Dot(right_normal_) < -radius) return false;
 	if (local_center.Dot(left_normal_) < -radius) return false;
	return true;
}

// See ViewPoint.h.
void Camera::Activate(int x, int y, int w, int h) {
	// Calculate the aspect ratio and normals
	float near_height = float(tan(field_of_view_ * kPi / 360 ) * near_plane_ / 2);
	float near_width = near_height * float(w) / h;
	front_normal_ = GetForwards();
  back_normal_ = -GetForwards();
	top_normal_ = (near_plane_*GetUp() + GetForwards()*near_height).GetNormal();
	bottom_normal_ = (-near_plane_*GetUp() + GetForwards()*near_height).GetNormal();
	right_normal_ = (near_plane_*GetRight() + GetForwards()*near_width).GetNormal();
	left_normal_ = (-near_plane_*GetRight() + GetForwards()*near_width).GetNormal();

  // Push and set the projection matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
  glViewport(x, gWindow->GetHeight() - y - h, w, h);
	glFrustum(-near_width, near_width, -near_height, near_height, near_plane_, far_plane_);

	// Push and set the modelview matrix (converted to right-hand coordinates)
	glMatrixMode(GL_MODELVIEW);	
	glPushMatrix();
	glLoadIdentity();
	glScalef(1, 1, -1);
	static float transformation[16];
	transformation[0] = GetRight()[0];
	transformation[4] = GetRight()[1];
	transformation[8] = GetRight()[2];
	transformation[1] = GetUp()[0];
	transformation[5] = GetUp()[1];
	transformation[9] = GetUp()[2];
	transformation[2] = GetForwards()[0];
	transformation[6] = GetForwards()[1];
	transformation[10] = GetForwards()[2];
  transformation[3] = transformation[7] = transformation[11]
	                  = transformation[12] = transformation[13] = transformation[14] = 0;
	transformation[15] = 1;
	glMultMatrixf(transformation);
	glTranslatef(-GetPosition()[0], -GetPosition()[1], -GetPosition()[2]);

	// Activate other Gl settings
	if (is_fog_enabled_) {
    glFogfv(GL_FOG_COLOR, fog_color_.GetData());
		glFogf(GL_FOG_START, fog_start_);
		glFogf(GL_FOG_END, fog_end_);
		glFogi(GL_FOG_MODE, GL_LINEAR);
		glEnable(GL_FOG);
	}
	glDepthMask(true);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
}

// See ViewPoint.h.
void Camera::Deactivate() {
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glDisable(GL_FOG);
	glDepthMask(false);
	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, gWindow->GetWidth(), gWindow->GetHeight());   
  front_normal_ = back_normal_ = top_normal_ = bottom_normal_
                = right_normal_ = left_normal_ = Vec3();
}
