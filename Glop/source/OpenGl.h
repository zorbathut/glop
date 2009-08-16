// This file has two purposes:
//   - Including all Open GL header files in an OS-independent way.
//   - Providing a small set of additional Gl primitives (e.g. rectangle, textures, etc.)

#ifndef GLOP_OPEN_GL_H__
#define GLOP_OPEN_GL_H__

// Windows includes
#ifdef WIN32
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#undef CreateWindow  // Stupid Microsoft and its stupid macros
#undef GetCharWidth
#undef GetMessage
#undef MessageBox
#endif

// Mac OSX includes
#ifdef MACOSX
#include <OpenGl/gl.h>
#include <OpenGl/glu.h>
#endif

#ifdef LINUX
#include <GL/gl.h>
#include <GL/glu.h>
#endif

// Other includes
#include "Base.h"
#include "Color.h"
#include "Image.h"
#include "List.h"
#include "Stream.h"

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
  static Texture *Load(InputStream input, int mag_filter = GL_LINEAR,
                       int min_filter = GL_LINEAR);
  static Texture *Load(InputStream input, const Color &bg_color, int bg_tolerance,
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
  ListId glop_index_;
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
  ListId glop_index_;
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
  ListId glop_index_;
  DISALLOW_EVIL_CONSTRUCTORS(DisplayLists);
};

// GlUtils class definition
class GlUtils {
 public:
  static void SetColor(const Color &color) {
    glColor4fv(color.GetData());
  }
  static void SetTexture(const Texture *texture) {
    ASSERT(texture->gl_id_);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture->gl_id_);
  }
  static void SetNoTexture() {
    glDisable(GL_TEXTURE_2D);
  }
};

// GlUtils2d class definition. This class contains a variety of pixel-accurate rendering tools in
// 2 dimensions. For example: DrawLine(50, 50, 100, 100) is guaranteed to go between pixels (50,50)
// and (100,100). This is essentially implemented by artificially increasing the size of all
// rectangles by 1, and by artificially translating all lines by 0.5.
class GlUtils2d {
 public:
  // Draws a line between the two given point. All textures should be deactivated.
  static void DrawLine(int x1, int y1, int x2, int y2);
  static void DrawLine(int x1, int y1, int x2, int y2, const Color &color) {
    GlUtils::SetColor(color);
    DrawLine(x1, y1, x2, y2);
  }

  // Draws a rectangle between the two given point. All textures should be deactivated.
  static void DrawRectangle(int x1, int y1, int x2, int y2);
  static void DrawRectangle(int x1, int y1, int x2, int y2, const Color &color) {
    GlUtils::SetColor(color);
    DrawRectangle(x1, y1, x2, y2);
  }

  // Draws a filled rectangle between the two given points in the current color. All textures
  // should be deactivated.
  static void FillRectangle(int x1, int y1, int x2, int y2);
  static void FillRectangle(int x1, int y1, int x2, int y2, const Color &color) {
    GlUtils::SetColor(color);
    FillRectangle(x1, y1, x2, y2);
  }

  // Draws a rectangle in the given color, tiled with the given texture. The texture is
  // deactivated when done. Since this is tiled, the texture should have width and height a power
  // of 2 or the result will look wrong.
  static void TileRectangle(int x1, int y1, int x2, int y2, const Texture *texture,
                            const Color &color = kWhite) {
    RenderTexture(x1, y1, x2, y2, 0, 0,
                  (x2 - x1 + 1.0f) / texture->GetWidth(),
                  (y2 - y1 + 1.0f) / texture->GetHeight(),
                  false, texture, color);
  }

  // Texture rendering. Render a full texture in the current color, optionally scaling it to fit
  // within the given rectangle. Works even if the texture does not have width and height a power
  // of 2.
  static void RenderTexture(int x1, int y1, const Texture *texture, const Color &color = kWhite) {
    RenderTexture(x1, y1, x1 + texture->GetWidth() - 1, y1 + texture->GetHeight() - 1,
                  texture, color);
  }
  static void RenderTexture(int x1, int y1, int x2, int y2, const Texture *texture,
                            const Color &color = kWhite) {
    RenderTexture(x1, y1, x2, y2, 0, 0,
                  float(texture->GetWidth()) / texture->GetInternalWidth(),
                  float(texture->GetHeight()) / texture->GetInternalHeight(),
                  true, texture, color);
  }

  // Partial or tiled texture rendering. Render some part of a texture in the current color,
  // scaling it to fit within the given rectangle. The texture coordinates can optionally be
  // clamped to [0,1]. If the texture is not tiled, it is important to do this. Otherwise, the
  // border pixels will blend with the border pixels from the opposite side.
  static void RenderTexture(int x1, int y1, int x2, int y2,
                            float tu1, float tv1, float tu2, float tv2, bool clamp,
                            const Texture *texture, const Color &color = kWhite);

  // Rotated texture rendering. Renders an entire texture (TODO: rotated sub-texture support) at
  // an angle. The base coordinates specify the rectangle that the texture should occupy when there
  // is no rotation and horz_flip can be used to flip the texture horizontally being rotating. (A
  // vertical flip can be accomplished by a horizontal flip and an extra 180 degree rotation.)
  //
  // The rotation itself can be specified in two ways. First of all, a clockwise rotation angle can
  // be specified. Secondly, the up-direction can be specified as a 2d vector. This way is faster
  // but the user should ensure that up has length 1.
  static void RenderRotatedTexture(int base_x1, int base_y1, int base_x2, int base_y2,
                                   float degrees, bool horz_flip, const Texture *texture,
                                   const Color &color = kWhite);
  static void RenderRotatedTexture(int base_x1, int base_y1, int base_x2, int base_y2,
                                   float up_x, float up_y, bool horz_flip, const Texture *texture,
                                   const Color &color = kWhite);
};

#endif // GLOP_OPEN_GL_H__
