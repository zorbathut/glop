// Includes
#include "../include/Font.h"
#include "../include/GlopWindow.h"
#include "../include/OpenGl.h"
#include "GlopInternalData.h"
#include "third_party/freetype/ftglyph.h"
#include "third_party/freetype/ftoutln.h"

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

FontBitmap *FontOutline::AddRef(int size, unsigned int flags) {
  const int kScale = 80;

  // Check if the bitmap is already loaded. Note that underline is ignored by the FontBitmap, so
  // we can ignore that flag.
  pair<int, unsigned int> key = make_pair(size, flags & (~kFontUnderline));
  if (bitmaps_.count(key)) {
    bitmaps_[key]->ref_count_++;
    return bitmaps_[key];
  }

  // Data
  int x1[kNumFontCharacters], y1[kNumFontCharacters],
      x2[kNumFontCharacters], y2[kNumFontCharacters];
  int dx[kNumFontCharacters];
  unsigned char *bitmaps[kNumFontCharacters];
  bool error = false;
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
  bitmaps_[key] = result;

  // Clean up
  for (int i = 0; i < kNumFontCharacters; i++)
  if (bitmaps[i] != 0)
    delete[] bitmaps[i];
  return result;
}

void FontOutline::FreeRef(int size, unsigned int flags) {
  pair<int, unsigned int> key = make_pair(size, flags & (~kFontUnderline));
  ASSERT(bitmaps_.count(key) > 0);
  bitmaps_[key]->ref_count_--;
  if (bitmaps_[key]->ref_count_ == 0) {
    delete bitmaps_[key];
    bitmaps_.erase(key);
  }
}

FontOutline::FontOutline(unsigned char *data, void *face): data_(data), face_(face) {}

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
          memcpy(data_ + (y+ty)*width_ + x, bitmaps[i] + ty*w, w);
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
      data_ = new unsigned char[width_ * height_];
    }
  }
  texture_ = new Texture(data_, width_, height_, 8, GL_NEAREST, GL_NEAREST);
}

FontBitmap::~FontBitmap() {
  delete texture_;
  delete[] data_;
}

// Font
// ====

Font *Font::Load(BinaryFileReader reader) {
  FontOutline *outline = FontOutline::Load(reader);
  return (outline != 0? new Font(outline, true) : 0);
}

Font::~Font() {
  if (is_outline_owned_)
    delete outline_;
  ASSERT(renderers_.size() == 0);
}

TextRenderer *Font::AddRef(int size, const Color &color, unsigned int flags) {
  pair<pair<int, Color>, unsigned int> key = make_pair(make_pair(size, color), flags);
  if (renderers_.count(key))
    renderers_[key]->ref_count_++;
  else
    renderers_[key] = new TextRenderer(this, outline_->AddRef(size, flags), size, color, flags);
  return renderers_[key];
}

void Font::FreeRef(int size, const Color &color, unsigned int flags) {
  pair<pair<int, Color>, unsigned int> key = make_pair(make_pair(size, color), flags);
  ASSERT(renderers_.count(key) > 0);
  renderers_[key]->ref_count_--;
  if (renderers_[key]->ref_count_ == 0) {
    delete renderers_[key];
    outline_->FreeRef(size, flags);
    renderers_.erase(key);
  }
}

void Font::RenderChar(const FontBitmap *bitmap, const Texture *bitmap_texture, const Color &color,
                      char ch) const {
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

void Font::RenderUnderline(const FontBitmap *bitmap, int x, int y, int len,
                           const Color &color) const {
  int y1 = bitmap->GetUnderlineStart() + y, y2 = y1 + bitmap->GetUnderlineHeight() - 1;
  GlUtils2d::FillRectangle(x, y1, x+len-1, y2, color);
}

// TextRendererDisplayLists
// ========================
//
// A series of display lists that renders each character in a font, advancing the cursor each time.
class TextRendererDisplayLists: public DisplayLists {
 public:
  TextRendererDisplayLists(const Font *font, const FontBitmap *bitmap,
                           const Texture *bitmap_texture, const Color &color)
  : DisplayLists(kNumFontCharacters), font_(font), bitmap_(bitmap),
    bitmap_texture_(bitmap_texture), color_(color) {}

 protected:
  void Render(int i) const {
    font_->RenderChar(bitmap_, bitmap_texture_, color_, i);
    glTranslatef((float)font_->GetDx(bitmap_, i), 0, 0);
  }

 private:
  const Font *font_;
  const FontBitmap *bitmap_;
  const Texture *bitmap_texture_;
  Color color_;
  DISALLOW_EVIL_CONSTRUCTORS(TextRendererDisplayLists);
};

// TextRenderer
// ============

void TextRenderer::FreeRef(TextRenderer *renderer) {
  renderer->font_->FreeRef(renderer->size_, renderer->color_, renderer->flags_);
}

void TextRenderer::Print(int x, int y, const string &text) const {
  if (text.size() > 0) {
    // Adjust so that x and y are the baseline beginning
    x -= GetX1(text[0]);
    y += GetAscent();

    // Begin with the underline - this way the font overlaps the underline, not vice-versa
    GlUtils::SetColor(color_);
    if ((flags_ & kFontUnderline) > 0) {
      GlUtils::SetNoTexture();
      int w = GetTextWidth(text);
      font_->RenderUnderline(bitmap_, x, y, w, color_);
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

TextRenderer::TextRenderer(Font *font, FontBitmap *bitmap, int size, const Color &color,
                           unsigned int flags)
: font_(font), bitmap_(bitmap), size_(size), color_(color), flags_(flags), ref_count_(1) {
  display_lists_ = new TextRendererDisplayLists(font, bitmap, bitmap->texture_, color);
}

TextRenderer::~TextRenderer() {
  delete display_lists_;
}

// ShadowFont
// ==========

ShadowFont *ShadowFont::Load(BinaryFileReader reader, float shadow_dx, float shadow_dy,
                             const Color &shadow_color) {
  FontOutline *outline = FontOutline::Load(reader);
  return (outline != 0? new ShadowFont(outline, true, shadow_dx, shadow_dy, shadow_color) : 0);
}

void ShadowFont::RenderChar(const FontBitmap *bitmap, const Texture *bitmap_texture,
                            const Color &color, char ch) const {
  GlUtils::SetColor(shadow_color_);
  glPushMatrix();
  glTranslatef((float)GetShadowDx(bitmap), (float)GetShadowDy(bitmap), 0);
  Font::RenderChar(bitmap, bitmap_texture, shadow_color_, ch);
  glPopMatrix();
  GlUtils::SetColor(color);
  Font::RenderChar(bitmap, bitmap_texture, shadow_color_, ch);
}

void ShadowFont::RenderUnderline(const FontBitmap *bitmap, int x, int y, int len,
                                 const Color &color) const {
  GlUtils::SetColor(shadow_color_);
  Font::RenderUnderline(bitmap, x + GetShadowDx(bitmap), y + GetShadowDy(bitmap), len,
                        shadow_color_);
  GlUtils::SetColor(color);
  Font::RenderUnderline(bitmap, x, y, len, color);
}

int ShadowFont::GetShadowDx(const FontBitmap *bitmap) const {
  float temp = bitmap->GetAscent() * shadow_dx_;
  return (temp < 0? int(temp-1) : int(temp+1));
}

int ShadowFont::GetShadowDy(const FontBitmap *bitmap) const {
  float temp = bitmap->GetAscent() * shadow_dy_;
  return (temp < 0? int(temp-1) : int(temp+1));
}
