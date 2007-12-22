// Includes
#include "../../include/glop3d/Camera.h"
#include "../../include/GlopWindow.h"
#include "../../include/OpenGl.h"
#include <cmath>

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
  glScissor(cx1, gWindow->GetHeight() - 1 - cy2, cx2 - cx1 + 1, cy2 - cy1 + 1);

  // Draw a fog background
  if (is_fog_enabled_) {
    GlUtils2d::FillRectangle(GetX(), GetY(), GetX2(), GetY2(), fog_color_);
    GlUtils::SetColor(kWhite);
  }

  // Activate the camera
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
  glViewport(GetX(), gWindow->GetHeight() - GetY() - GetHeight(), GetWidth(), GetHeight());
  float near_height = float(tan(camera().GetFieldOfView() * kPi / 360 ) *
                            camera().GetNearPlane() / 2);
	float near_width = near_height * float(GetWidth()) / GetHeight();
  glFrustum(-near_width, near_width, -near_height, near_height, camera_.GetNearPlane(),
            camera_.GetFarPlane());

	glMatrixMode(GL_MODELVIEW);	
	glPushMatrix();
	glLoadIdentity();
	glScalef(1, 1, -1);
	static float transformation[16];
	transformation[0] = camera().right()[0];
	transformation[4] = camera().right()[1];
	transformation[8] = camera().right()[2];
	transformation[1] = camera().up()[0];
	transformation[5] = camera().up()[1];
	transformation[9] = camera().up()[2];
	transformation[2] = camera().forwards()[0];
	transformation[6] = camera().forwards()[1];
	transformation[10] = camera().forwards()[2];
  transformation[3] = transformation[7] = transformation[11]
	                  = transformation[12] = transformation[13] = transformation[14] = 0;
	transformation[15] = 1;
	glMultMatrixf(transformation);
	glTranslatef(-camera().position()[0], -camera().position()[1], -camera().position()[2]);

	glDepthMask(true);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);

  // Activate fog if that is enabled
	if (is_fog_enabled_) {
    glFogfv(GL_FOG_COLOR, fog_color_.GetData());
		glFogf(GL_FOG_START, fog_start_);
		glFogf(GL_FOG_END, fog_end_);
		glFogi(GL_FOG_MODE, GL_LINEAR);
		glEnable(GL_FOG);
	}

  // Render the scene
  Render3d();

  // Reset everything
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glDepthMask(false);
	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, gWindow->GetWidth(), gWindow->GetHeight());   
  if (old_clipping_enabled)
    glScissor(old_scissor_test[0], old_scissor_test[1], old_scissor_test[2], old_scissor_test[3]);
  else
    glDisable(GL_SCISSOR_TEST);
}

bool CameraFrame::IsInFrustum(const Point3 &center, float radius) const {
	Point3 local_center = center - camera_.position();
	float z_dist = local_center.Dot(front_normal_);
  if (z_dist+radius < camera_.GetNearPlane() || z_dist-radius > camera_.GetFarPlane() ||
      (is_fog_enabled_ && z_dist-radius > fog_end_))
    return false;
  if (local_center.Dot(top_normal_) < -radius) return false;
	if (local_center.Dot(bottom_normal_) < -radius) return false;
	if (local_center.Dot(right_normal_) < -radius) return false;
 	if (local_center.Dot(left_normal_) < -radius) return false;
	return true;
}

void CameraFrame::UpdateNormals() {
  float near_height = float(tan(camera().GetFieldOfView() * kPi / 360 ) *
                            camera().GetNearPlane() / 2);
	float near_width = near_height * float(GetWidth()) / GetHeight();
	front_normal_ = camera().forwards();
  back_normal_ = -camera().forwards();
	left_normal_ = (-camera().GetNearPlane()*camera().right() +
                  camera().forwards()*near_width).GetNormal();
	right_normal_ = (camera().GetNearPlane()*camera().right() +
                   camera().forwards()*near_width).GetNormal();
	top_normal_ = (camera().GetNearPlane()*camera().up() +
                 camera().forwards()*near_height).GetNormal();
	bottom_normal_ = (-camera().GetNearPlane()*camera().up() +
                    camera().forwards()*near_height).GetNormal();
}