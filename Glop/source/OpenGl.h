// This file has two purposes:
//   - Including all Open GL header files in an OS-independent way.
//   - Providing a small set of additional Gl primitives (e.g. rectangle, line of text, etc.)

#ifndef GLOP_OPEN_GL_H__
#define GLOP_OPEN_GL_H__

// Windows Gl includes
#ifdef WIN32
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#undef CreateWindow  // Stupid Microsoft and its stupid macros
#undef GetCharWidth
#endif
#ifdef MACOSX
#include <OpenGl/gl.h>
#include <OpenGl/glu.h>
#endif


// Other includes
#include "Base.h"
#include "Color.h"
#include "System.h"

// GlUtils class definition
class GlUtils {
 public:
  // Sets the current Open Gl color and texture mapping properties.
  static void SetColor(const Color &color) {
    glColor4fv(color.GetData());
  }
  static void SetNoTexture() {
    glDisable(GL_TEXTURE_2D);
  }
  static void SetFontTexture(LightSetId font_id) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gSystem->GetFontGlId(font_id));
  }
  static void SetTexture(LightSetId texture_id) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gSystem->GetTextureGlId(texture_id));
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

  // Draws the given image between the two given points in the current color. The active texture
  // will be changed to the given texture. Blending is not automatically enabled even for
  // translucent images.
  static void RenderImage(int x1, int y1, int x2, int y2, LightSetId texture_id);

  // Prints the given line of text at the given point. Assumes the texture has already been set
  // and blending has already been enabled.
  static void Print(int x, int y, const string &text, LightSetId font_id);
};

#endif // GLOP_OPEN_GL_H__