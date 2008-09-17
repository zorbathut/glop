// Includes
#include "OpenGl.h"
#include "GlopWindow.h"
#include "Image.h"
#include "GlopInternalData.h"

// Texture class
// =============

Texture *Texture::Load(BinaryFileReader reader, int mag_filter, int min_filter) {
  Image *image = Image::Load(reader);
  if (image == 0)
    return 0;
  Texture *texture = new Texture(image, mag_filter, min_filter);
  texture->is_image_owned_ = true;
  return texture;
}

Texture *Texture::Load(BinaryFileReader reader, const Color &bg_color, int bg_tolerance,
                       int mag_filter, int min_filter) {
  Image *image = Image::Load(reader, bg_color, bg_tolerance);
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

  if ((mag_filter_ != GL_NEAREST || mag_filter_ != GL_LINEAR) &&
      (min_filter_ != GL_NEAREST || min_filter_ != GL_LINEAR)) {
    glTexImage2D(GL_TEXTURE_2D, 0, format, GetInternalWidth(), GetInternalHeight(), 0, format,
                 GL_UNSIGNED_BYTE, image_->Get());
  } else {
    gluBuild2DMipmaps(GL_TEXTURE_2D, format, GetInternalWidth(), GetInternalHeight(), format,
                      GL_UNSIGNED_BYTE, image_->Get());
  }
  glDisable(GL_TEXTURE_2D);
}

void Texture::GlShutDown() {
  if (gl_id_ != 0)
    glDeleteTextures(1, &gl_id_);
  gl_id_ = 0;
}

// DisplayList class
// =================

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

// GlUtils2d class
// ===============

// The line-drawing convention appears to be as follows: If x1 and x2 are different, one is
// subtracted from the max. Otherwise, they remain the same. If y1 and y2 are different, one is
// subtracted from the max. Otherwise, one is subtracted from both.
void GlUtils2d::DrawLine(int x1, int y1, int x2, int y2) {
  if (x2 >= x1) x2++; else x1++;
  if (y2 >= y1) y2++; else y1++;
  glBegin(GL_LINES);
  glVertex2i(x1, y1);
  glVertex2i(x2, y2);
  glEnd();
}

// See DrawLine conventions.
void GlUtils2d::DrawRectangle(int x1, int y1, int x2, int y2) {
  int minx = min(x1, x2), maxx = max(x1, x2);
  int miny = min(y1, y2), maxy = max(y1, y2);
  glBegin(GL_LINE_LOOP);
  glVertex2i(minx+1, miny);
  glVertex2i(maxx+1, miny+1);
  glVertex2i(maxx, maxy+1);
  glVertex2i(minx, maxy);
  glEnd();
}

void GlUtils2d::FillRectangle(int x1, int y1, int x2, int y2) {
  int min_x = min(x1, x2), max_x = max(x1, x2);
  int min_y = min(y1, y2), max_y = max(y1, y2);
  glBegin(GL_QUADS);
  glVertex2i(min_x, min_y);
  glVertex2i(max_x + 1, min_y);
  glVertex2i(max_x + 1, max_y + 1);
  glVertex2i(min_x, max_y + 1);
  glEnd();
}

void GlUtils2d::FillRectangleTexture(int x1, int y1, int x2, int y2, const Texture *texture)
{
  int min_x = min(x1, x2), max_x = max(x1, x2);
  int min_y = min(y1, y2), max_y = max(y1, y2);
  int num_tiles_x = (max_x - min_x)/texture->GetWidth();
  int num_tiles_y = (max_y - min_y)/texture->GetHeight();
  float x_extra = (max_x - min_x)%texture->GetWidth()/(float)texture->GetWidth();
  float y_extra = (max_y - min_y)%texture->GetHeight()/(float)texture->GetHeight();
  float x_scale = float(texture->GetWidth()) / texture->GetInternalWidth();
  float y_scale = float(texture->GetHeight()) / texture->GetInternalHeight();
  RenderSubTexture(x1, y1, x2, y2, 0, 0, x_scale * num_tiles_x + x_scale * x_extra, y_scale * num_tiles_y + y_scale * y_extra, false, texture);
}

void GlUtils2d::RenderSubTexture(int x1, int y1, int x2, int y2, float tu1, float tv1, float tu2,
                                 float tv2, bool clamp, const Texture *texture) {
  int min_x = min(x1, x2), max_x = max(x1, x2);
  int min_y = min(y1, y2), max_y = max(y1, y2);
  GlUtils::SetTexture(texture);
  if (clamp) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);  // Prevents texture bleeding across
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);  //  top & bottom
  }
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
}
