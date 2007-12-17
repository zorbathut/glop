// This file has two purposes:
//   - Including all Open GL header files in an OS-independent way.
//   - Providing a small set of additional Gl primitives (e.g. rectangle, line of text, etc.)

#ifndef GLOP_OPEN_GL_H__
#define GLOP_OPEN_GL_H__

// Windows includes
#ifdef WIN32
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#undef CreateWindow  // Stupid Microsoft and its stupid macros
#undef GetCharWidth
#endif

// Mac OSX includes
#ifdef MACOSX
#include <OpenGl/gl.h>
#include <OpenGl/glu.h>
#endif

// Other includes
#include "Base.h"
#include "Color.h"
#include "System.h"

// OpenGL data objects. Note that switching into and out of fullscreen mode invalidates textures
// and display lists. To prevent that from happening, these structures can and should be used
// instead. They are a very light wrapper around the OpenGL functions, except they will always
// stay valid.
class Texture {
 public:
  Texture(const void *data, int width, int height, int bpp, int mag_filter, int min_filter);
  ~Texture();
  int GetWidth() const {return width_;}
  int GetHeight() const {return height_;}
  int GetBpp() const {return bpp_;}
 
 private:
  friend class GlDataManager;
  void GlInit();
  void GlShutDown();

  friend class GlUtils;
  unsigned int gl_id_;

  const void *data_;
  int width_, height_, bpp_;
  int mag_filter_, min_filter_;
  LightSetId glop_index_;
  DISALLOW_EVIL_CONSTRUCTORS(Texture);
};

class DisplayList {
 public:
  virtual ~DisplayList();
  void Call();
  void Clear();
 protected:
  DisplayList();
  virtual void Render() const = 0;
 private:
  unsigned int gl_id_;
  LightSetId glop_index_;
  DISALLOW_EVIL_CONSTRUCTORS(DisplayList);
};

class DisplayLists {
 public:
  virtual ~DisplayLists();
  void Call(int num_lists, GLenum index_type, const GLvoid *indices);
  void Clear();
 protected:
  DisplayLists(int n);
  virtual void Render(int i) const = 0;
 private:
  int n_;
  unsigned int base_gl_id_;
  LightSetId glop_index_;
  DISALLOW_EVIL_CONSTRUCTORS(DisplayLists);
};

// GlUtils class definition
class GlUtils {
 public:
  static void SetColor(const Color &color) {
    glColor4fv(color.GetData());
  }
  static void SetTexture(const Texture *texture) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture->gl_id_);
  }
  static void SetNoTexture() {
    glDisable(GL_TEXTURE_2D);
  }
};

// GlUtils2d class definition
class GlUtils2d {
 public:
  // Draws a line between the two given points in the current color. Texturing should be disabled.
  // A color may be optionally set before the rectangle is drawn.
  static void DrawLine(int x1, int y1, int x2, int y2);
  static void DrawLine(int x1, int y1, int x2, int y2, const Color &color) {
    GlUtils::SetColor(color);
    DrawLine(x1, y1, x2, y2);
  }

  // Draws a hollow rectangle between the two given points in the current color. Texturing should
  // be disabled. A color may be optionally set before the rectangle is drawn.
  static void DrawRectangle(int x1, int y1, int x2, int y2);
  static void DrawRectangle(int x1, int y1, int x2, int y2, const Color &color) {
    GlUtils::SetColor(color);
    DrawRectangle(x1, y1, x2, y2);
  }

  // Draws a filled rectangle between the two given points in the current color. Texturing should
  // be disabled. A color may be optionally set before the rectangle is drawn.
  static void FillRectangle(int x1, int y1, int x2, int y2);
  static void FillRectangle(int x1, int y1, int x2, int y2, const Color &color) {
    GlUtils::SetColor(color);
    FillRectangle(x1, y1, x2, y2);
  }
};

#endif // GLOP_OPEN_GL_H__
