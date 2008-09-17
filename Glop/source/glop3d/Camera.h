// The most basic tools for rendering in 3d.
//
// Camera - A viewpoint combined with field of view and near+far planes.
// CameraFrame - A GlopFrame that executes the virtual function Render3d after setting up the
//               view matrices for rendering a 3d scene.

#ifndef GLOP_GLOP3D_CAMERA_H__
#define GLOP_GLOP3D_CAMERA_H__

// Includes
#include "Point3.h"
#include "../Color.h"
#include "../GlopFrameBase.h"

// Class declarations
class GlopWindow;

// Camera class definition
class Camera: public Viewpoint {
 public:
  // Constructors - field_of_view is the vertical angle spanned by the top and bottom planes seen.
	Camera(const Viewpoint &view_point = Viewpoint())
  : Viewpoint(view_point),
    near_plane_(0.1f),
    far_plane_(150.0f),
    field_of_view_(60.0f) {}
  const Camera &operator=(const Camera &rhs) {
    Viewpoint::operator=(rhs);
    near_plane_ = rhs.near_plane_;
    far_plane_ = rhs.far_plane_;
    field_of_view_ = rhs.field_of_view_;
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

  // Moves and rotates the camera so it is looking directly at the plane spanned by the given
  // points, and such that the top and bottom are the topmost and bottommost points visible on
  // the plane. The left and right bounds are used to center the camera, but we cannot guarantee
  // they will be the exact leftmost and rightmost bounds until we know the aspect ratio to render
  // in. The top and left edges should be perpendicular.
  void LookAt(const Point3 &top_left, const Point3 &top_right, const Point3 &bottom_left);

 private:
  float near_plane_, far_plane_, field_of_view_;
};

// CameraFrame class definition
class CameraFrame: public GlopFrame {
 public:
  CameraFrame(const Camera &camera = Camera())
  : aspect_ratio_(-1), camera_(camera), is_fog_enabled_(false) {}

  // Switches between 3d coordinates and screen coordinates.
  void Project(const Point3 &val, int *x, int *y) const;
  Point3 Unproject(int x, int y, float depth) const;

  // This function can be used to ensure the CameraFrame maintains an aspect ratio (width / height)
  // as given. LookAt uses this to ensure the CameraFrame views precisely the given rectangle
  // without distortion.
  void FixAspectRatio(float aspect_ratio) {
    aspect_ratio_ = aspect_ratio;
    DirtySize();
  }
  void LookAt(const Point3 &top_left, const Point3 &top_right, const Point3 &bottom_left,
              float field_of_view);
  void LookAt(const Point3 &top_left, const Point3 &top_right, const Point3 &bottom_left);

  // Rendering. Render3d should be overloaded, NOT render.
  void Render() const;

  // Camera control
  const Camera &GetCamera() const {return camera_;}
  void SetCamera(const Camera &camera) {
    camera_ = camera;
    UpdateNormals();
  }

  // Fog control
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

  // Frustum info
  bool IsInFrustum(const Point3 &center, float radius) const;
  const Vec3 &GetFrontNormal() const {return front_normal_;}
  const Vec3 &GetBackNormal() const {return back_normal_;}
  const Vec3 &GetLeftNormal() const {return left_normal_;}
  const Vec3 &GetRightNormal() const {return right_normal_;}
  const Vec3 &GetTopNormal() const {return top_normal_;}
  const Vec3 &GetBottomNormal() const {return bottom_normal_;}

 protected:
  virtual void Render3d() const = 0;
  void RecomputeSize(int rec_width, int rec_height);

 private:
  void UpdateNormals();

  float aspect_ratio_;
  Camera camera_;
  bool is_fog_enabled_;
  float fog_start_, fog_end_;
  Color fog_color_;
  Vec3 front_normal_, back_normal_, left_normal_, right_normal_, top_normal_, bottom_normal_;
  DISALLOW_EVIL_CONSTRUCTORS(CameraFrame);
};

#endif // GLOP_GLOP3D_CAMERA_H__
