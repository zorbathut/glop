// Includes
#include "Font.h"
#include "GlopInternalData.h"
#include "GlopWindow.h"
#include "OpenGl.h"
#include "third_party/freetype/ftglyph.h"
#include "third_party/freetype/ftoutln.h"
#include <map>
using namespace std;

// FontOutline
// ===========

FontOutline *FontOutline::Load(BinaryFileReader reader) {
  // First make sure the file is valid
  if (!reader.IsOpen())
    return 0;

  // Load the font face, and make sure it is a scalable font
  int data_length = reader.GetLength();
  unsigned char *data = new unsigned char[data_length];
  FT_Face face = 0;
  if (reader.ReadChars(data_length, data) < data_length ||
      FT_New_Memory_Face((FT_Library)FreeTypeLibrary::Get(), data, data_length, 0, &face) ||
      !FT_IS_SCALABLE(face)) {
    if (face != 0)
      FT_Done_Face(face);
    delete[] data;
    return 0;
  } else {
    return new FontOutline(data, face);
  }
}

FontOutline::~FontOutline() {
  map<pair<int, unsigned int>, FontBitmap*> *bm_map =
    (map<pair<int, unsigned int>, FontBitmap*>*)bitmaps_;
  ASSERT(bm_map->size() == 0);
  delete bm_map;
  FT_Done_Face((FT_Face)face_);
  delete[] data_;
}

FontBitmap *FontOutline::AddRef(int size, unsigned int flags) {
  const int kScale = 80;

  // Check if the bitmap is already loaded. Note that underline is ignored by the FontBitmap, so
  // we can ignore that flag.
  map<pair<int, unsigned int>, FontBitmap*> *bm_map =
    (map<pair<int, unsigned int>, FontBitmap*>*)bitmaps_;
  pair<int, unsigned int> key = make_pair(size, flags & (~kFontUnderline));
  if (bm_map->count(key)) {
    (*bm_map)[key]->ref_count_++;
    return (*bm_map)[key];
  }

  // Data
  int x1[kNumFontCharacters], y1[kNumFontCharacters],
      x2[kNumFontCharacters], y2[kNumFontCharacters];
  int dx[kNumFontCharacters];
  unsigned char *bitmaps[kNumFontCharacters];
  FT_Face face = (FT_Face)face_;

  // Set the size for this FreeType font. Note that we need to scale by dpi.
  FT_Set_Char_Size(face, 0, size*kScale, 0, 0);

  // Load bitmaps for all characters
  for (int i = 0; i < kNumFontCharacters; i++) {
    bitmaps[i] = 0;
    if (FT_Load_Glyph(face, FT_Get_Char_Index(face, i), FT_LOAD_DEFAULT))
      continue;

    // Add a transformation for italics
    if ((flags & kFontItalics) > 0) {
	    FT_Matrix shear;
	    shear.xx = 1 << 16;
	    shear.xy = (int)(0.3f * (1 << 16));
	    shear.yx = 0;
	    shear.yy = 1 << 16;
      FT_Outline_Transform(&face->glyph->outline, &shear);
    }

    // Create the character bitmap
    if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL))
      continue;

    // Store the character settings
    x1[i] = face->glyph->bitmap_left;
    y1[i] = -face->glyph->bitmap_top;
    int w = face->glyph->bitmap.width, h = face->glyph->bitmap.rows;
    x2[i] = x1[i] + w - 1;
    y2[i] = y1[i] + h - 1;
    dx[i] = (face->glyph->advance.x / 64);

    // Store the character bitmap
    bitmaps[i] = new unsigned char[w * h];
    memcpy(bitmaps[i], face->glyph->bitmap.buffer, w * h);
  }

  // Adjust the data for bold fonts
  if ((flags & kFontBold) > 0) {
    // Calculate the bold padding
    int hpad = 0;
    for (int i = 0; i < kNumFontCharacters; i++)
    if (bitmaps[i] != 0)
      hpad = max(hpad, (y2[i] - y1[i]) / 30 + 1);

    // Adjust everything
    for (int i = 0; i < kNumFontCharacters; i++)
    if (bitmaps[i] != 0) {
      int w = x2[i] - x1[i] + 1, h = y2[i] - y1[i] + 1;
      x2[i] += hpad;
      dx[i] += hpad;
      unsigned char *new_bm = new unsigned char[(w + hpad) * h];
      for (int y = 0; y < h; y++) {
        int brightness = 0;
        for (int x = 0; x < w + hpad; x++) {
          if (x < w)
            brightness += bitmaps[i][y*w + x];
          if (x > hpad)
            brightness -= bitmaps[i][y*w + x-hpad-1];
          new_bm[y*(w+hpad)+x] = (unsigned char)min(brightness, 255);
        }
      }
      delete bitmaps[i];
      bitmaps[i] = new_bm;
    }
  }

  // Create the font bitmap
  FontBitmap *result = new FontBitmap(bitmaps, x1, y1, x2, y2, dx);
  (*bm_map)[key] = result;

  // Clean up
  for (int i = 0; i < kNumFontCharacters; i++)
  if (bitmaps[i] != 0)
    delete[] bitmaps[i];
  return result;
}

void FontOutline::FreeRef(int size, unsigned int flags) {
  map<pair<int, unsigned int>, FontBitmap*> *bm_map =
    (map<pair<int, unsigned int>, FontBitmap*>*)bitmaps_;
  pair<int, unsigned int> key = make_pair(size, flags & (~kFontUnderline));
  ASSERT(bm_map->count(key) > 0);
  (*bm_map)[key]->ref_count_--;
  if ((*bm_map)[key]->ref_count_ == 0) {
    delete (*bm_map)[key];
    bm_map->erase(key);
  }
}

FontOutline::FontOutline(unsigned char *data, void *face)
: bitmaps_((void*)new map<pair<int, unsigned int>, FontBitmap*>()), face_(face), data_(data) {}

// FontBitmap
// ==========

void FontBitmap::GetTexCoords(char ch, float *tu1, float *tv1, float *tu2, float *tv2) const {
  *tu1 = float(char_bitmap_x_[ch]) / width_;
  *tv1 = float(char_bitmap_y_[ch]) / height_;
  *tu2 = float(char_bitmap_x_[ch]+char_x2_[ch]-char_x1_[ch]+1) / width_;
  *tv2 = float(char_bitmap_y_[ch]+char_y2_[ch]-char_y1_[ch]+1) / height_;
}

FontBitmap::FontBitmap(unsigned char **bitmaps, int *x1, int *y1, int *x2, int *y2, int *dx)
: ref_count_(1) {
  // Store the overall font metrics
  descent_ = ascent_ = 0;
  for (int i = 0; i < kNumFontCharacters; i++)
  if (bitmaps[i] != 0) {
    ascent_ = max(ascent_, -y1[i]);
    descent_ = max(descent_, y2[i]);
  }

  // Calculate the underline position - TrueType fonts have some info on this, but it doesn't
  // seem very good, so we can just calculate our own.
  ul_start_ = descent_/3;
  ul_height_ = max(descent_/2 - ul_start_, 1);

  // Store the character metrics
  memset(char_bitmap_x_, -1, kNumFontCharacters*sizeof(char_bitmap_x_[0]));
  memset(char_bitmap_y_, -1, kNumFontCharacters*sizeof(char_bitmap_y_[0]));
  memcpy(char_x1_, x1, kNumFontCharacters*sizeof(x1[0])); 
  memcpy(char_y1_, y1, kNumFontCharacters*sizeof(y1[0]));
  memcpy(char_x2_, x2, kNumFontCharacters*sizeof(x2[0])); 
  memcpy(char_y2_, y2, kNumFontCharacters*sizeof(y2[0]));
  memcpy(char_dx_, dx, kNumFontCharacters*sizeof(dx[0]));

  // Copy all the character bitmaps into a single big bitmap. Do two passes - one to find the size
  // for the main bitmap, and one to copy data into the bitmap.
  width_ = height_ = 64;
  for (int pass = 0; pass < 2; pass++) {
restart:;
    int x = 0, y = 0, row_height = 0;
    for (int i = 0; i < kNumFontCharacters; i++)
    if (bitmaps[i] != 0) {
      int w = x2[i] - x1[i] + 1, h = y2[i] - y1[i] + 1;

      // Position this bitmap, and possibly exit early
      if (x + w > width_) {
        x = 0;
        y += row_height;
        row_height = 0;
      }
      if (y + h > height_ || x + w > width_) {
        width_ *= 2;
        height_ *= 2;
        ASSERT(pass == 0); // Our size should already be set before the second pass
        goto restart;
      }

      // Place this character
      if (pass == 1) {
        char_bitmap_x_[i] = x;
        char_bitmap_y_[i] = y;
        for (int ty = 0; ty < h; ty++)
          memcpy(image_->Get(x, y+ty), bitmaps[i] + ty*w, w);
      }

      // Move to the next character
      x += w;
      row_height = max(row_height, h);
    }

    // If we successfully finished the first pass, allocate memory. Also, decrease the height as
    // long as everything fits. Now, in the second pass, we will actually data onto our bitmaps.
    if (pass == 0) {
      while (y+row_height <= height_/2)
        height_ /= 2;
      image_ = new Image(width_, height_, 8);
    }
  }
  texture_ = new Texture(image_);
}

FontBitmap::~FontBitmap() {
  delete texture_;
  delete image_;
}

// Font
// ====

Font *Font::Load(BinaryFileReader reader) {
  FontOutline *outline = FontOutline::Load(reader);
  return (outline != 0? new Font(outline, true) : 0);
}

Font::Font(FontOutline *outline)
: renderers_((void*)new map<pair<int, unsigned int>, TextRenderer*>()),
  outline_(outline), is_outline_owned_(false) {}

Font::Font(FontOutline *outline, bool is_outline_owned)
: renderers_((void*)new map<pair<int, unsigned int>, TextRenderer*>()),
  outline_(outline), is_outline_owned_(is_outline_owned) {}

Font::~Font() {
  map<pair<int, unsigned int>, TextRenderer*> *rend_map =
    (map<pair<int, unsigned int>, TextRenderer*>*)renderers_;
  if (is_outline_owned_)
    delete outline_;
  ASSERT(rend_map->size() == 0);
  delete rend_map;
}

TextRenderer *Font::AddRef(int size, unsigned int flags) {
  map<pair<int, unsigned int>, TextRenderer*> *rend_map =
    (map<pair<int, unsigned int>, TextRenderer*>*)renderers_;
  pair<int, unsigned int> key = make_pair(size, flags);
  if (rend_map->count(key))
    (*rend_map)[key]->ref_count_++;
  else
    (*rend_map)[key] = new TextRenderer(this, outline_->AddRef(size, flags), size, flags);
  return (*rend_map)[key];
}

void Font::FreeRef(int size, unsigned int flags) {
  map<pair<int, unsigned int>, TextRenderer*> *rend_map =
    (map<pair<int, unsigned int>, TextRenderer*>*)renderers_;
  pair<int, unsigned int> key = make_pair(size, flags);
  ASSERT(rend_map->count(key) > 0);
  (*rend_map)[key]->ref_count_--;
  if ((*rend_map)[key]->ref_count_ == 0) {
    delete (*rend_map)[key];
    outline_->FreeRef(size, flags);
    rend_map->erase(key);
  }
}

void Font::RenderChar(const FontBitmap *bitmap, const Texture *bitmap_texture, char ch) const {
  int x1 = bitmap->GetX1(ch), y1 = bitmap->GetY1(ch),
      x2 = bitmap->GetX2(ch)+1, y2 = bitmap->GetY2(ch)+1;
  float tu1, tv1, tu2, tv2;
  bitmap->GetTexCoords(ch, &tu1, &tv1, &tu2, &tv2);
  glBegin(GL_QUADS);
  glTexCoord2f(tu1, tv1);
  glVertex2i(x1, y1);
  glTexCoord2f(tu2, tv1);
  glVertex2i(x2, y1);
  glTexCoord2f(tu2, tv2);
  glVertex2i(x2, y2);
  glTexCoord2f(tu1, tv2);
  glVertex2i(x1, y2);
  glEnd();
}

void Font::RenderUnderline(const FontBitmap *bitmap, int x, int y, int len) const {
  int y1 = bitmap->GetUnderlineStart() + y, y2 = y1 + bitmap->GetUnderlineHeight() - 1;
  GlUtils2d::FillRectangle(x, y1, x+len-1, y2);
}

// TextRendererDisplayLists
// ========================
//
// A series of display lists that renders each character in a font, advancing the cursor each time.
class TextRendererDisplayLists: public DisplayLists {
 public:
  TextRendererDisplayLists(const Font *font, const FontBitmap *bitmap,
                           const Texture *bitmap_texture)
  : DisplayLists(kNumFontCharacters), font_(font), bitmap_(bitmap),
    bitmap_texture_(bitmap_texture) {}

 protected:
  void Render(int i) const {
    font_->RenderChar(bitmap_, bitmap_texture_, i);
    glTranslatef((float)font_->GetDx(bitmap_, i), 0, 0);
  }

 private:
  const Font *font_;
  const FontBitmap *bitmap_;
  const Texture *bitmap_texture_;
  DISALLOW_EVIL_CONSTRUCTORS(TextRendererDisplayLists);
};

// TextRenderer
// ============

void TextRenderer::FreeRef(TextRenderer *renderer) {
  renderer->font_->FreeRef(renderer->size_, renderer->flags_);
}

void TextRenderer::Print(int x, int y, const string &text, const Color &color) const {
  if (text.size() == 0)
    return;

  // Set up fog
  bool is_fog_enabled = (glIsEnabled(GL_FOG) == GL_TRUE);
  int fog_mode;
  float fog_color[4], fog_start, fog_end;
  if (is_fog_enabled) {
    glGetFloatv(GL_FOG_COLOR, fog_color);
    glGetFloatv(GL_FOG_START, &fog_start);
    glGetFloatv(GL_FOG_END, &fog_end);
    glGetIntegerv(GL_FOG_MODE, &fog_mode);
  } else {
    glEnable(GL_FOG);
  }
  glFogfv(GL_FOG_COLOR, kBlack.GetData());
	glFogf(GL_FOG_START, 0);
	glFogf(GL_FOG_END, 1);
	glFogi(GL_FOG_MODE, GL_LINEAR);

  // Adjust so that x and y are the baseline beginning
  x -= GetX1(text[0]);
  y += GetAscent();

  // Begin with the underline - this way the font overlaps the underline, not vice-versa
  GlUtils::SetColor(color);
  if ((flags_ & kFontUnderline) > 0) {
    GlUtils::SetNoTexture();
    int w = GetTextWidth(text);
    font_->RenderUnderline(bitmap_, x, y, w);
  }

  // Render the text
  glEnable(GL_BLEND);
  GlUtils::SetTexture(bitmap_->texture_);
  glPushMatrix();
  glTranslatef(float(x), float(y), 0);
  display_lists_->Call((int)text.size(), GL_BYTE, text.c_str());
  glPopMatrix();

  // Clear the settings
  GlUtils::SetNoTexture();
  glDisable(GL_BLEND);
  glDisable(GL_FOG);
  if (is_fog_enabled) {
    glFogfv(GL_FOG_COLOR, fog_color);
	  glFogf(GL_FOG_START, fog_start);
	  glFogf(GL_FOG_END, fog_end);
	  glFogi(GL_FOG_MODE, fog_mode);
  }
}

// Generally, a character reserves the x-coordinates between where it starts rendering, and where
// the next character starts rendering. However, we tweak this for the first and last character so
// that those characters also claim the extra overhang on the left and right.
int TextRenderer::GetCharWidth(char ch, bool is_first_char, bool is_last_char) const {
  int result = GetDx(ch);
  if (is_last_char)
    result = max(result, GetX2(ch) + 1);
  if (is_first_char)
    result -= GetX1(ch);
  return result;
}

int TextRenderer::GetTextWidth(const string &s, bool is_first_text, bool is_last_text) const {
  int total_width = 0, len = (int)s.size();
  for (int i = 0; i < len; i++)
    total_width += GetCharWidth(s[i], i == 0 && is_first_text, i == len - 1 && is_last_text);
  return total_width;
}

TextRenderer::TextRenderer(Font *font, FontBitmap *bitmap, int size, unsigned int flags)
: font_(font), bitmap_(bitmap), size_(size), flags_(flags), ref_count_(1) {
  display_lists_ = new TextRendererDisplayLists(font, bitmap, bitmap->texture_);
}

TextRenderer::~TextRenderer() {
  delete display_lists_;
}

// GradientFont
// ============

GradientFont *GradientFont::Load(BinaryFileReader reader, float top_brightness,
                                 float bottom_brightness, const vector<float> &mid_pos,
                                 const vector<float> &mid_brightness) {
  FontOutline *outline = FontOutline::Load(reader);
  return (outline != 0? new GradientFont(outline, true, top_brightness, bottom_brightness,
                                         mid_pos, mid_brightness) : 0);
}

GradientFont::GradientFont(FontOutline *outline, float top_brightness, float bottom_brightness)
: Font(outline) {
  Init(top_brightness, bottom_brightness, vector<float>(0), vector<float>(0));
}

GradientFont::GradientFont(FontOutline *outline, float top_brightness, float bottom_brightness,
                           float mid_pos, float mid_brightness): Font(outline) {
  Init(top_brightness, bottom_brightness, vector<float>(1, mid_pos),
       vector<float>(1, mid_brightness));
}

GradientFont::GradientFont(FontOutline *outline, float top_brightness, float bottom_brightness,
                           const vector<float> &mid_pos, const vector<float> &mid_brightness)
: Font(outline) {
  Init(top_brightness, bottom_brightness, mid_pos, mid_brightness);
}

GradientFont::GradientFont(FontOutline *outline, bool is_outline_owned, float top_brightness,
                           float bottom_brightness, const vector<float> &mid_pos,
                           const vector<float> &mid_brightness)
: Font(outline, is_outline_owned) {
  Init(top_brightness, bottom_brightness, mid_pos, mid_brightness);
}

void GradientFont::RenderChar(const FontBitmap *bitmap, const Texture *bitmap_texture,
                              char ch) const {
  int x1 = bitmap->GetX1(ch), y1 = bitmap->GetY1(ch),
      x2 = bitmap->GetX2(ch), y2 = bitmap->GetY2(ch);
  vector<int> ypos;
  vector<float> yc;
  GetColors(bitmap, y1, y2, &ypos, &yc);
  float tu1, tv1, tu2, tv2;
  bitmap->GetTexCoords(ch, &tu1, &tv1, &tu2, &tv2);
  glBegin(GL_QUAD_STRIP);
  for (int i = 0; i < (int)ypos.size(); i++) {
    float tv = (y2 == y1? tv1 : tv1 + (ypos[i] - y1) * (tv2 - tv1) / (y2 - y1));
    glTexCoord2f(tu1, tv); 
    glVertex3f(float(x1), float(ypos[i] + (i == 0? 0 : 1)), 1 - yc[i]);
    glTexCoord2f(tu2, tv);
    glVertex3f(float(x2 + 1), float(ypos[i] + (i == 0? 0 : 1)), 1 - yc[i]);
  }
  glEnd();
}

void GradientFont::RenderUnderline(const FontBitmap *bitmap, int x, int y, int len) const {
  int y1 = bitmap->GetUnderlineStart(), y2 = y1 + bitmap->GetUnderlineHeight() - 1;
  vector<int> ypos;
  vector<float> yc;
  GetColors(bitmap, y1, y2, &ypos, &yc);
  glBegin(GL_QUAD_STRIP);
  for (int i = 0; i < (int)ypos.size(); i++) {
    glVertex3f(float(x), y + float(ypos[i] + (i == 0? 0 : 1)), 1 - yc[i]);
    glVertex3f(float(x + len), y + float(ypos[i] + (i == 0? 0 : 1)), 1 - yc[i]);
  }
  glEnd();
}

void GradientFont::Init(float top_brightness, float bottom_brightness, const vector<float> &mid_pos,
                        const vector<float> &mid_brightness) {
  ASSERT(mid_pos.size() == mid_brightness.size());
  brightness_pos_.push_back(-1);
  brightness_.push_back(top_brightness);
  for (int i = 0; i < (int)mid_pos.size(); i++) {
    ASSERT(mid_pos[i] > -1 && mid_pos[i] < 1 && (i == 0 || mid_pos[i] < mid_pos[i-1]));
    brightness_pos_.push_back(mid_pos[i]);
    brightness_.push_back(mid_brightness[i]);
  }
  brightness_pos_.push_back(1);
  brightness_.push_back(bottom_brightness);
}

void GradientFont::GetColors(const FontBitmap *bitmap, int y1, int y2, vector<int> *y,
                             vector<float> *yc) const {
  y->clear();
  yc->clear();
  
  vector<int> brightness_pixel;
  for (int i = 0; i < (int)brightness_pos_.size(); i++) {
    int scale = (brightness_pos_[i] < 0? bitmap->GetAscent() : bitmap->GetDescent());
    brightness_pixel.push_back(int(scale * brightness_pos_[i]));
  }

  for (int i = 0; i < (int)brightness_pixel.size(); i++) {
    if (brightness_pixel[i] > y1 && y->size() == 0) {
      y->push_back(y1);
      yc->push_back(brightness_[i-1] + (brightness_[i] - brightness_[i-1]) *
                    (y1 - brightness_pixel[i-1]) / (brightness_pixel[i] - brightness_pixel[i-1]));
    }
    if (brightness_pixel[i] >= y1 && brightness_pixel[i] <= y2) {
      y->push_back(brightness_pixel[i]);
      yc->push_back(brightness_[i]);
    }
    if (brightness_pixel[i] > y2) {
      y->push_back(y2);
      yc->push_back(brightness_[i-1] + (brightness_[i] - brightness_[i-1]) *
                    (y2 - brightness_pixel[i-1]) / (brightness_pixel[i] - brightness_pixel[i-1]));
      break;
    }
  }
}

// ShadowFont
// ==========

ShadowFont *ShadowFont::Load(BinaryFileReader reader, float shadow_dx, float shadow_dy,
                             float shadow_brightness) {
  FontOutline *outline = FontOutline::Load(reader);
  return (outline != 0?
    new ShadowFont(outline, true, shadow_dx, shadow_dy, shadow_brightness) : 0);
}

void ShadowFont::RenderChar(const FontBitmap *bitmap, const Texture *bitmap_texture,
                            char ch) const {
  int x1 = bitmap->GetX1(ch), y1 = bitmap->GetY1(ch),
      x2 = bitmap->GetX2(ch)+1, y2 = bitmap->GetY2(ch)+1;
  float dx = float(GetShadowDx(bitmap)), dy = float(GetShadowDy(bitmap)),
        dz = 1 - shadow_brightness_;
  float tu1, tv1, tu2, tv2;
  bitmap->GetTexCoords(ch, &tu1, &tv1, &tu2, &tv2);

  // Render the shadow
  glBegin(GL_QUADS);
  glTexCoord2f(tu1, tv1);
  glVertex3f(x1 + dx, y1 + dy, dz);
  glTexCoord2f(tu2, tv1);
  glVertex3f(x2 + dx, y1 + dy, dz);
  glTexCoord2f(tu2, tv2);
  glVertex3f(x2 + dx, y2 + dy, dz);
  glTexCoord2f(tu1, tv2);
  glVertex3f(x1 + dx, y2 + dy, dz);

  // Render the main character
  glTexCoord2f(tu1, tv1);
  glVertex2i(x1, y1);
  glTexCoord2f(tu2, tv1);
  glVertex2i(x2, y1);
  glTexCoord2f(tu2, tv2);
  glVertex2i(x2, y2);
  glTexCoord2f(tu1, tv2);
  glVertex2i(x1, y2);
  glEnd();
}

void ShadowFont::RenderUnderline(const FontBitmap *bitmap, int x, int y, int len) const {
  int y1 = y + bitmap->GetUnderlineStart(), y2 = y1 + bitmap->GetUnderlineHeight() - 1;
  float dx = float(GetShadowDx(bitmap)), dy = float(GetShadowDy(bitmap)),
        dz = 1 - shadow_brightness_;
  glBegin(GL_QUADS);
  glVertex3f(x + dx, y1 + dy, dz);
  glVertex3f(x + len + dx, y1 + dy, dz);
  glVertex3f(x + len + dx, y2 + dy, dz);
  glVertex3f(x + dx, y2 + dy, dz);
  glVertex2i(x, y1);
  glVertex2i(x + len, y1);
  glVertex2i(x + len, y2);
  glVertex2i(x, y2);
  glEnd();
}

int ShadowFont::GetShadowDx(const FontBitmap *bitmap) const {
  float temp = bitmap->GetAscent() * shadow_dx_;
  return (temp < 0? int(temp-1) : int(temp+1));
}

int ShadowFont::GetShadowDy(const FontBitmap *bitmap) const {
  float temp = bitmap->GetAscent() * shadow_dy_;
  return (temp < 0? int(temp-1) : int(temp+1));
}
