// Simple 3d objects: a ViewPoint (position + an orientation) and a Camera (a ViewPoint + fog,
// field of view, and clipping planes).

#ifndef GLOP_GLOP3D_VIEW_POINT_H__
#define GLOP_GLOP3D_VIEW_POINT_H__

// Includes
#include "Point3.h"
#include "../Color.h"

// Class declarations
class GlopWindow;

// ViewPoint class definition. This is a position plus a view orientation.
class ViewPoint {
 public:
  // Constructors. In general, one constructs a ViewPoint from a forward vector and a requested
  // up vector (which is implicitly (0, 1, 0) if nothing is specified). It is guaranteed that the
  // actual up vector will be in the same plane as the forward vector and the requested up vector.
  // Thus ViewPoint(x) creates a view point with no roll facing towards x.
  ViewPoint()
  : position_(),
    forward_vector_(0,0,1),
    up_vector_(0,1,0),
    right_vector_(1,0,0) {}
  ViewPoint(const Point3 &position)
  : position_(position),
    forward_vector_(0,0,1),
    up_vector_(0,1,0),
    right_vector_(1,0,0) {}
  ViewPoint(const Point3 &position, const Vec3 &forward_vector, const Vec3 &up_vector)
  : position_(position) {
    SetDirection(forward_vector, up_vector);
  }
  ViewPoint(const ViewPoint &rhs)
  : position_(rhs.position_),
    forward_vector_(rhs.forward_vector_),
    up_vector_(rhs.up_vector_),
    right_vector_(rhs.right_vector_) {}
  void operator=(const ViewPoint &rhs) {
    position_ = rhs.position_;
    forward_vector_ = rhs.forward_vector_;
    up_vector_ = rhs.up_vector_;
    right_vector_ = rhs.right_vector_;
  }

	// Accessors
	const Point3 &GetPosition() const {return position_;}
	const Vec3 &GetForwards() const {return forward_vector_;}
	const Vec3 &GetUp() const {return up_vector_;}
	const Vec3 &GetRight() const {return right_vector_;}

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

  // Coordinate transformations. Each ViewPoint can be thought of specifying its own coordinate
  // system. Here, we provide utilities for switching between that coordinate system and the
  // "global" one (i.e., the normal, base coordinate system).
	Point3 LocalToGlobal(const Point3 &p) const;
	Point3 GlobalToLocal(const Point3 &p) const;
	ViewPoint LocalToGlobal(const ViewPoint &vp) const;
	ViewPoint GlobalToLocal(const ViewPoint &vp) const;
  void FillTransformationMatrix(float *matrix) const;

  // Comparators
  bool operator==(const ViewPoint &rhs) const {
    return position_ == rhs.position_ &&
           forward_vector_ == rhs.forward_vector_ &&
           up_vector_ == rhs.up_vector_ &&
           right_vector_ == rhs.right_vector_;
  }
  bool operator!=(const ViewPoint &rhs) const {
    return !operator==(rhs);
  }

 private:
  // Private constructor - used by the coordinate transformations. This is more efficient than
  // constructing a blank ViewPoint and filling its data manually.
  ViewPoint(const Point3 &position, const Vec3 &forward_vector, const Vec3 &up_vector,
            const Vec3 &right_vector)
  : position_(position),
    forward_vector_(forward_vector),
    up_vector_(up_vector),
    right_vector_(right_vector) {}

	Point3 position_;
  Vec3 forward_vector_, up_vector_, right_vector_;
};

// Camera class definition
class Camera: public ViewPoint {
 public:
  // Constructors
	Camera(const ViewPoint &view_point = ViewPoint())
  : ViewPoint(view_point),
    near_plane_(0.1f),
    far_plane_(150.0f),
    field_of_view_(90.0f),
    is_fog_enabled_(false) {}
  const Camera &operator=(const Camera &rhs) {
    ViewPoint::operator=(rhs);
    near_plane_ = rhs.near_plane_;
    far_plane_ = rhs.far_plane_;
    field_of_view_ = rhs.field_of_view_;
    is_fog_enabled_ = rhs.is_fog_enabled_;
    fog_start_ = rhs.fog_start_;
    fog_end_ = rhs.fog_end_;
    fog_color_ = rhs.fog_color_;
	return rhs;
  }
  Camera(const Camera &rhs) {
    operator=(rhs);
  }

	// Accessors/mutators. Field of view is in degrees.
  float GetFieldOfView() const {return field_of_view_;}
	void SetFieldOfView(float degrees) {field_of_view_ = degrees;}
	float GetNearPlane() const {return near_plane_;}
	void SetNearPlane(float dist) {near_plane_ = dist;}
	float GetFarPlane() const {return far_plane_;}
	void SetFarPlane(float dist) {far_plane_ = dist;}
	bool IsFogEnabled() const {return is_fog_enabled_;}
	const Color &GetFogColor() const {return fog_color_;}
	float GetFogStartDistance() const {return fog_start_;}
	float GetFogEndDistance() const {return fog_end_;}
	void SetFog(const Color &color, float start_distance, float end_distance) {
    is_fog_enabled_ = true;
    fog_color_ = color;
    fog_start_ = start_distance;
    fog_end_ = end_distance;
  }
  void ClearFog() {is_fog_enabled_ = false;}

  // Frustum information. These values depend on the aspect ratio of the display frame, so they
  // are undefined unless the camera is active. Technically: the normals are set on a call to
  // Activate and cleared on a call to Deactivate.
  bool IsInFrustum(const Point3 &center, float radius) const;
  const Vec3 &GetFrontNormal() const {return front_normal_;}
  const Vec3 &GetBackNormal() const {return back_normal_;}
  const Vec3 &GetTopNormal() const {return top_normal_;}
  const Vec3 &GetBottomNormal() const {return bottom_normal_;}
  const Vec3 &GetRightNormal() const {return right_normal_;}
  const Vec3 &GetLeftNormal() const {return left_normal_;}

  // Configures OpenGl to render a scene from the perspective of this camera (and with extra
  // display settings as set on this camera). The output will be of width w and height h starting
  // at the position (x, y). Note: this sets the normal values used above.
	void Activate(int x, int y, int w, int h);

  // Undoes the effect of an Activate call. Note: this clears the normal values used above.
	void Deactivate();
	
 private:
  float near_plane_, far_plane_, field_of_view_;
  bool is_fog_enabled_;
  float fog_start_, fog_end_;
  Color fog_color_;
  Vec3 front_normal_, back_normal_, top_normal_, bottom_normal_, right_normal_, left_normal_;
};

#endif // GLOP_GLOP3D_VIEW_POINT_H__
