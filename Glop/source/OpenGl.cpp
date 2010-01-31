// Includes
#include "OpenGl.h"
#include "GlopInternalData.h"
#include "GlopWindow.h"
#include "Image.h"
#include <cmath>

// Texture class
// =============

Texture *Texture::Load(InputStream input, int mag_filter, int min_filter) {
  Image *image = Image::Load(input);
  if (image == 0)
    return 0;
  Texture *texture = new Texture(image, mag_filter, min_filter);
  texture->is_image_owned_ = true;
  return texture;
}

Texture *Texture::Load(InputStream input, const Color &bg_color, int bg_tolerance,
                       int mag_filter, int min_filter) {
  Image *image = Image::Load(input, bg_color, bg_tolerance);
  if (image == 0)
    return 0;
  Texture *texture = new Texture(image, mag_filter, min_filter);
  texture->is_image_owned_ = true;
  return texture;
}

Texture::Texture(const Image *image, int mag_filter, int min_filter)
: image_(image), is_image_owned_(false), mag_filter_(mag_filter), min_filter_(min_filter),
  gl_id_(0) {
  ASSERT(image != 0);
  glop_index_ = GlDataManager::RegisterTexture(this);
  if (window()->IsCreated())
    GlInit();
}

Texture::~Texture() {
  GlShutDown();
  GlDataManager::UnregisterTexture(glop_index_);
  if (is_image_owned_)
    delete image_;
}

void Texture::GlInit() {
  const int kFormatList[] = {GL_ALPHA, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA};
  ASSERT(gl_id_ == 0);
  int format = kFormatList[GetBpp()/8 - 1];
  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &gl_id_);
  glBindTexture(GL_TEXTURE_2D, gl_id_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter_);

  #ifndef IPHONE
  if ((mag_filter_ != GL_NEAREST || mag_filter_ != GL_LINEAR) &&
      (min_filter_ != GL_NEAREST || min_filter_ != GL_LINEAR)) {
    gluBuild2DMipmaps(GL_TEXTURE_2D, format, GetInternalWidth(), GetInternalHeight(), format,
                      GL_UNSIGNED_BYTE, image_->Get());
  } else {
  #endif
    glTexImage2D(GL_TEXTURE_2D, 0, format, GetInternalWidth(), GetInternalHeight(), 0, format,
                 GL_UNSIGNED_BYTE, image_->Get());
  #ifndef IPHONE
  }
  #endif
  glDisable(GL_TEXTURE_2D);
}

void Texture::GlShutDown() {
  if (gl_id_ != 0)
    glDeleteTextures(1, &gl_id_);
  gl_id_ = 0;
}

// DisplayList class
// =================
#ifndef IPHONE
DisplayList::~DisplayList() {
  Clear();
  GlDataManager::UnregisterDisplayList(glop_index_);
}

void DisplayList::Call() {
  if (gl_id_ == 0) {
    gl_id_ = glGenLists(1);
    glNewList(gl_id_, GL_COMPILE_AND_EXECUTE);
    Render();
    glEndList();
  } else {
    glCallList(gl_id_);
  }
}

void DisplayList::Clear() {
  if (gl_id_ != 0)
    glDeleteLists(gl_id_, 1);
  gl_id_ = 0;
}

DisplayList::DisplayList(): gl_id_(0) {
  glop_index_ = GlDataManager::RegisterDisplayList(this);
}

// DisplayList class
// =================

DisplayLists::~DisplayLists() {
  Clear();
  GlDataManager::UnregisterDisplayLists(glop_index_);
}

void DisplayLists::Call(int num_lists, GLenum index_type, const GLvoid *indices) {
  if (base_gl_id_ == 0) {
    base_gl_id_ = glGenLists(n_);
    for (int i = 0; i < n_; i++) {
      glNewList(base_gl_id_ + i, GL_COMPILE);
      Render(i);
      glEndList();
    }
  }
  glListBase(base_gl_id_);
  glCallLists(num_lists, index_type, indices);
}

void DisplayLists::Clear() {
  if (base_gl_id_ != 0)
    glDeleteLists(base_gl_id_, n_);
  base_gl_id_ = 0;
}

DisplayLists::DisplayLists(int n): n_(n), base_gl_id_(0) {
  glop_index_ = GlDataManager::RegisterDisplayLists(this);
}
#endif

// GlUtils2d class
// ===============

// The end vertices don't always get rendered with GL_LINES
void GlUtils2d::DrawLine(int x1, int y1, int x2, int y2) {
#ifdef IPHONE
  ASSERT(0);
#else
  glBegin(GL_LINES);
  glVertex2f(x1+0.5f, y1+0.5f);
  glVertex2f(x2+0.5f, y2+0.5f);
  glEnd();
  glBegin(GL_POINTS);
  glVertex2f(x1+0.5f, y1+0.5f);
  glVertex2f(x2+0.5f, y2+0.5f);
  glEnd();
#endif
}

void GlUtils2d::DrawRectangle(int x1, int y1, int x2, int y2) {
#ifdef IPHONE
  ASSERT(0);
#else
  glBegin(GL_LINE_LOOP);
  glVertex2f(x1+0.5f, y1+0.5f);
  glVertex2f(x2+0.5f, y1+0.5f);
  glVertex2f(x2+0.5f, y2+0.5f);
  glVertex2f(x1+0.5f, y2+0.5f);
  glEnd();
#endif
}

void GlUtils2d::FillRectangle(int x1, int y1, int x2, int y2) {
#ifdef IPHONE
  ASSERT(0);
#else
  int min_x = min(x1, x2), max_x = max(x1, x2);
  int min_y = min(y1, y2), max_y = max(y1, y2);
  glBegin(GL_QUADS);
  glVertex2i(min_x, min_y);
  glVertex2i(max_x + 1, min_y);
  glVertex2i(max_x + 1, max_y + 1);
  glVertex2i(min_x, max_y + 1);
  glEnd();
#endif
}

void GlUtils2d::RenderTexture(int x1, int y1, int x2, int y2,
                              float tu1, float tv1, float tu2, float tv2, bool clamp,
                              const Texture *texture, const Color &color) {
#ifdef IPHONE
  ASSERT(0);
#else
  GlUtils::SetColor(color);
  GlUtils::SetTexture(texture);
  if (clamp) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  }

  int min_x = min(x1, x2), max_x = max(x1, x2);
  int min_y = min(y1, y2), max_y = max(y1, y2);
  glBegin(GL_QUADS);
  glTexCoord2f(tu1, tv1);
  glVertex2i(min_x, min_y);
  glTexCoord2f(tu2, tv1);
  glVertex2i(max_x + 1, min_y);
  glTexCoord2f(tu2, tv2);
  glVertex2i(max_x + 1, max_y + 1);
  glTexCoord2f(tu1, tv2);
  glVertex2i(min_x, max_y + 1);
  glEnd();

  if (clamp) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }
  GlUtils::SetNoTexture();
#endif
}

void GlUtils2d::RenderRotatedTexture(int base_x1, int base_y1, int base_x2, int base_y2,
                                     float degrees, bool horz_flip, const Texture *texture,
                                     const Color &color) {
  float angle = kPi * degrees / 180.0f;
  RenderRotatedTexture(base_x1, base_y1, base_x2, base_y2,
                       sin(angle), -cos(angle), horz_flip, texture, color);
}

void GlUtils2d::RenderRotatedTexture(int base_x1, int base_y1, int base_x2, int base_y2,
                                     float up_x, float up_y, bool horz_flip,
                                     const Texture *texture, const Color &color) {
#ifdef IPHONE
  ASSERT(0);
#else
  GlUtils::SetColor(color);
  GlUtils::SetTexture(texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  int w = abs(base_x2 - base_x1) + 1, h = abs(base_x2 - base_x1) + 1;
  float x = (base_x1 + base_x2 + 1.0f) / 2, y = (base_y1 + base_y2 + 1.0f) / 2;
  float tu2 = float(texture->GetWidth()) / texture->GetInternalWidth(),
        tv2 = float(texture->GetHeight()) / texture->GetInternalHeight();
  float right_x = -up_y, right_y = up_x;
  float udx = up_x * h / 2, udy = up_y * h / 2;
  float rdx = right_x * w / 2, rdy = right_y * w / 2;

  glBegin(GL_QUADS);
  glTexCoord2f(horz_flip? tu2 : 0, 0);
  glVertex2f(x - rdx + udx, y - rdy + udy);
  glTexCoord2f(horz_flip? 0 : tu2, 0);
  glVertex2f(x + rdx + udx, y + rdy + udy);
  glTexCoord2f(horz_flip? 0 : tu2, tv2);
  glVertex2f(x + rdx - udx, y + rdy - udy);
  glTexCoord2f(horz_flip? tu2 : 0, tv2);
  glVertex2f(x - rdx - udx, y - rdy - udy);
  glEnd();

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  GlUtils::SetNoTexture();
#endif
}
