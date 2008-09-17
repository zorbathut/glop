// Includes
#include "glop3d/Camera.h"
#include "GlopWindow.h"
#include "OpenGl.h"
#include <cmath>

void Camera::LookAt(const Point3 &top_left, const Point3 &top_right, const Point3 &bottom_left) {
  float vert_dist = Dist(top_left, bottom_left);
  float eye_dist = (vert_dist / 2) / float(tan(GetFieldOfView() * kPi / 360));
  Point3 origin = (top_right + bottom_left) / 2;
  Point3 forward = Normalize(Cross(top_right - top_left, top_left - bottom_left));
  SetPosition(origin - forward * eye_dist);
  SetDirection(forward, top_left - bottom_left);
}

void CameraFrame::Project(const Point3 &val, int *x, int *y) const {
  Point3 local = camera_.GlobalToLocal(val);
  float height = float(tan(GetCamera().GetFieldOfView() * kPi / 360) * local[2]);
	float width = height * float(GetWidth()) / GetHeight();
  *x = int(GetX() + GetWidth() * (local[0] / width + 1)/2);
  *y = int(GetY2() - GetHeight() * (local[1] / height + 1)/2);
}

Point3 CameraFrame::Unproject(int x, int y, float depth) const {
  float x_frac = float(x - GetX()) / GetWidth();
  float y_frac = float(y - GetY()) / GetHeight();
  float height = float(tan(GetCamera().GetFieldOfView() * kPi / 360) * 2 * depth);
	float width = height * float(GetWidth()) / GetHeight();
  Point3 local((x_frac - 0.5f) * width, (0.5f - y_frac) * width, depth);
  return camera_.LocalToGlobal(local);
}

void CameraFrame::LookAt(const Point3 &top_left, const Point3 &top_right,
                         const Point3 &bottom_left, float field_of_view) {
  camera_.SetFieldOfView(field_of_view);
  LookAt(top_left, top_right, bottom_left);
}

void CameraFrame::LookAt(const Point3 &top_left, const Point3 &top_right,
                         const Point3 &bottom_left) {
  // We update our normals now even though it will also be done when the frame resizes so that
  // the correct normals will be available immediately.
  camera_.LookAt(top_left, top_right, bottom_left);
  FixAspectRatio((top_right - top_left).Norm() / (bottom_left - top_left).Norm());
  UpdateNormals();
}

void CameraFrame::Render() const {
  // Enable clipping - see ClippedFrame. We do it ourselves instead of actually using a
  // ClippedFrame so that the user can both extend CameraFrame and instantiate it without being
  // aware of any clipping nonsense.
  int old_scissor_test[4];
  int cx1 = max(GetClipX1(), GetX()), cy1 = max(GetClipY1(), GetY()),
      cx2 = min(GetClipX2(), GetX2()), cy2 = min(GetClipY2(), GetY2());
  if (cx1 > cx2 || cy1 > cy2)
    return;
  int old_clipping_enabled = glIsEnabled(GL_SCISSOR_TEST);
  if (old_clipping_enabled)
    glGetIntegerv(GL_SCISSOR_BOX, old_scissor_test);
  else
    glEnable(GL_SCISSOR_TEST);
  glScissor(cx1, window()->GetHeight() - 1 - cy2, cx2 - cx1 + 1, cy2 - cy1 + 1);

  // Draw the background - we can't just draw it in the fog color because, on some machines, the
  // fog will not actually fade things out to precisely the fog color. Instead, we render the
  // background in any color, but far into the fog.
  if (is_fog_enabled_) {
    glEnable(GL_FOG);
    glFogfv(GL_FOG_COLOR, fog_color_.GetData());
		glFogf(GL_FOG_START, -1);
		glFogf(GL_FOG_END, 0);
		glFogi(GL_FOG_MODE, GL_LINEAR);
  }
  GlUtils2d::FillRectangle(GetX(), GetY(), GetX2(), GetY2(), kBlack);

  // Activate the camera
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
  glViewport(GetX(), window()->GetHeight() - GetY() - GetHeight(), GetWidth(), GetHeight());
  float near_height = float(tan(GetCamera().GetFieldOfView() * kPi / 360 ) *
                            GetCamera().GetNearPlane());
	float near_width = near_height * float(GetWidth()) / GetHeight();
  glFrustum(-near_width, near_width, -near_height, near_height, camera_.GetNearPlane(),
            camera_.GetFarPlane());
	glMatrixMode(GL_MODELVIEW);	
	glPushMatrix();
	glLoadIdentity();
	glScalef(1, 1, -1);
	static float transformation[16];
	transformation[0] = GetCamera().right()[0];
	transformation[4] = GetCamera().right()[1];
	transformation[8] = GetCamera().right()[2];
	transformation[1] = GetCamera().up()[0];
	transformation[5] = GetCamera().up()[1];
	transformation[9] = GetCamera().up()[2];
	transformation[2] = GetCamera().forwards()[0];
	transformation[6] = GetCamera().forwards()[1];
	transformation[10] = GetCamera().forwards()[2];
  transformation[3] = transformation[7] = transformation[11]
	                  = transformation[12] = transformation[13] = transformation[14] = 0;
	transformation[15] = 1;
	glMultMatrixf(transformation);
	glTranslatef(-GetCamera().position()[0], -GetCamera().position()[1], -GetCamera().position()[2]);

	glDepthMask(true);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);

  // Activate fog if it is enabled. Note some of this was done above while drawing the background.
	if (is_fog_enabled_) {
		glFogf(GL_FOG_START, fog_start_);
		glFogf(GL_FOG_END, fog_end_);
	}

  // Render the scene
  Render3d();

  // Deactivate fog
  if (is_fog_enabled_)
    glDisable(GL_FOG);

  // Reset everything
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glDepthMask(false);
	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, window()->GetWidth(), window()->GetHeight());   
  if (old_clipping_enabled)
    glScissor(old_scissor_test[0], old_scissor_test[1], old_scissor_test[2], old_scissor_test[3]);
  else
    glDisable(GL_SCISSOR_TEST);
}

bool CameraFrame::IsInFrustum(const Point3 &center, float radius) const {
	Point3 local_center = center - camera_.position();
	float z_dist = Dot(local_center, front_normal_);
  if (z_dist+radius < camera_.GetNearPlane() || z_dist-radius > camera_.GetFarPlane() ||
      (is_fog_enabled_ && z_dist-radius > fog_end_))
    return false;
  if (Dot(local_center, top_normal_) < -radius) return false;
	if (Dot(local_center, bottom_normal_) < -radius) return false;
	if (Dot(local_center, right_normal_) < -radius) return false;
 	if (Dot(local_center, left_normal_) < -radius) return false;
	return true;
}

void CameraFrame::RecomputeSize(int rec_width, int rec_height) {
  if (aspect_ratio_ != -1)
    SetToMaxSize(rec_width, rec_height, aspect_ratio_);
  else
    SetSize(rec_width, rec_height);
  UpdateNormals();
}

void CameraFrame::UpdateNormals() {
  float near_height = float(tan(GetCamera().GetFieldOfView() * kPi / 360) *
                            GetCamera().GetNearPlane());
	float near_width = near_height * float(GetWidth()) / GetHeight();
	front_normal_ = GetCamera().forwards();
  back_normal_ = -GetCamera().forwards();
	left_normal_ = Normalize(-GetCamera().GetNearPlane()*GetCamera().right() +
                           GetCamera().forwards()*near_width);
	right_normal_ = Normalize(GetCamera().GetNearPlane()*GetCamera().right() +
                            GetCamera().forwards()*near_width);
	top_normal_ = Normalize(GetCamera().GetNearPlane()*GetCamera().up() +
                          GetCamera().forwards()*near_height);
	bottom_normal_ = Normalize(-GetCamera().GetNearPlane()*GetCamera().up() +
                             GetCamera().forwards()*near_height);
}
