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
#include "BinaryFileManager.h"
#include "Color.h"
#include "Image.h"

// OpenGL data objects. Note that switching into and out of fullscreen mode invalidates textures
// and display lists. To prevent that from happening, these structures can and should be used
// instead. They are a very light wrapper around the OpenGL functions, except they will always
// stay valid.

// An OpenGL texture.
class Texture {
 public:
  // Constructors. Load is similar to Image::Load. If this is used, the Image is owned by the
  // Texture and will be deleted automatically when the Texture is deleted. Otherwise, the user is
  // responsible for deleting the image.
  static Texture *Load(BinaryFileReader reader, int mag_filter = GL_LINEAR,
                       int min_filter = GL_LINEAR);
  static Texture *Load(BinaryFileReader reader, const Color &bg_color, int bg_tolerance,
                       int mag_filter = GL_LINEAR, int min_filter = GL_LINEAR);
  Texture(const Image *image, int mag_filter = GL_LINEAR, int min_filter = GL_LINEAR);
  ~Texture();

  // Accessors - See Image.
  int GetInternalWidth() const {return image_->GetInternalWidth();}
  int GetInternalHeight() const {return image_->GetInternalHeight();}
  int GetWidth() const {return image_->GetWidth();}
  int GetHeight() const {return image_->GetHeight();}
  int GetBpp() const {return image_->GetBpp();}
 
 private:
  friend class GlDataManager;
  void GlInit();
  void GlShutDown();

  friend class GlUtils;
  unsigned int gl_id_;

  const Image *image_;
  bool is_image_owned_;
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

  // Texture rendering without tiling:
  //  - The first function render the full texture (excluding padding to make its size a power of 2)
  //  - The second function renders the texture scaled, possibly distorting the aspect ratio
  //  - The subtexture functions are similar but they render any subrectangle of the texture
  // The texture is deactivated when done.
  static void RenderTexture(int x1, int y1, const Texture *texture) {
    RenderTexture(x1, y1, x1 + texture->GetWidth() - 1, y1 + texture->GetHeight() - 1, texture);
  }
  static void RenderTexture(int x1, int y1, int x2, int y2, const Texture *texture) {
    RenderSubTexture(x1, y1, x2, y2, 0, 0, float(texture->GetWidth()) / texture->GetInternalWidth(),
                     float(texture->GetHeight()) / texture->GetInternalHeight(), texture);
  }
  static void RenderSubTexture(int x1, int y1, float tu1, float tv1, float tu2, float tv2,
                               const Texture *texture) {
    RenderSubTexture(x1, y1, int(x1 + (tu2-tu1)*texture->GetInternalWidth() - 1),
                     int(y1 + (tv2-tv1)*texture->GetInternalHeight() - 1),
                     tu1, tv1, tu2, tv2, texture);
  }
  static void RenderSubTexture(int x1, int y1, int x2, int y2, float tu1, float tv1, float tu2,
                               float tv2, const Texture *texture);
};

#endif // GLOP_OPEN_GL_H__
