// Includes
#include "../include/OpenGl.h"
#include "../include/Image.h"

// GlUtils2d class
// ===============

// Either rounding in Open Gl is extremely badly done or the standard orthonormal projection matrix
// is wrong. Either way, we have to do absurd hacks to actually get the 2d primitives to render at
// precisely the right pixels.
void GlUtils2d::DrawLine(int x1, int y1, int x2, int y2) {
  if (x1 > x2 || (x1 == x2 && y1 > y2)) {
    int tx = x1; x1 = x2; x2 = tx;
    int ty = y1; y1 = y2; y2 = ty;
  }
  glBegin(GL_LINES);
  if (y1 < y2) {
    glVertex2i(x1, y1);
    glVertex2i(x2+1, y2+1);
  } else {
    glVertex2i(x1, y1+1);
    glVertex2i(x2+1, y2);
  }
  glEnd();
}

void GlUtils2d::DrawRectangle(int x1, int y1, int x2, int y2) {
  int minx = min(x1, x2), maxx = max(x1, x2);
  int miny = min(y1, y2), maxy = max(y1, y2);
  glBegin(GL_LINE_LOOP);
  glVertex2i(minx, miny+1);
  glVertex2i(maxx+1, miny+1);
  glVertex2i(maxx, maxy+1);
  glVertex2i(minx, maxy+1);
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

void GlUtils2d::RenderImage(int x1, int y1, int x2, int y2, LightSetId texture_id) {
  int min_x = min(x1, x2), max_x = max(x1, x2);
  int min_y = min(y1, y2), max_y = max(y1, y2);
  const Image *image = gSystem->GetTextureImage(texture_id);
  float tw = float(image->GetWidth()) / image->GetInternalWidth();
  float th = float(image->GetHeight()) / image->GetInternalHeight();
  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f);
  glVertex2f((float)min_x, (float)min_y);
  glTexCoord2f(tw, 0.0f);
  glVertex2f((float)max_x, (float)min_y);
  glTexCoord2f(tw, th);
  glVertex2f((float)max_x, (float)max_y);
  glTexCoord2f(0.0f, th);
  glVertex2f((float)min_x, (float)max_y);
  glEnd();
}

void GlUtils2d::Print(int x, int y, const string &text, LightSetId font_id) {
  if (text.size() > 0) {
    glPushMatrix();
    glTranslatef(float(x - gSystem->GetCharX1(font_id, text[0])), float(y), 0);
    glListBase(gSystem->GetFontDisplayList(font_id));
    glCallLists((int)text.size(), GL_BYTE, text.c_str());
    glPopMatrix();
  }
}
