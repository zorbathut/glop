// Utilities for loading TrueType fonts and quickly rendering them on-screen. Most applications
// should need only to create Font objects and pass those to Gui frames. Text output style can
// be customized by extending Font.
//
// TextRenderer - An interface for rendering text with a specific font, style, size, and color.
//                To actually print or get font metrics, you must use a TextRenderer.
//
// Font - From a basic perspective, a Font is a TextRenderer factory. A Font can be loaded
//        directly from a TrueType font file on disk, and it can instantiate TextRenderers for that
//        font with any style, size, and color.
//        Fonts are also important if you want to do custom text output. Ultimately a TextRenderer
//        is just data, and it delegates back to the Font for the rendering & metrics. Thus, these
//        functions can be customized by overloading Font.
//
// FontOutline - A direct representation of a TrueType font loaded from disk. This has no use
//               outside of Font, and a Font can load its own FontOutline if necessary. However,
//               if two Fonts are based on the same underline TrueType data, they can be created
//               from a shared FontOutline to decrease memory, CPU load.
//
// FontBitmap - A TrueType font converted into a giant bitmap for rendering purposes. Ultimately,
//              this will be used by Font both for rendering and for character metric queries.
//              Each TextRenderer will be given a FontBitmap, although several TextRenderer's could
//              possibly share a single TextRenderer.

#ifndef GLOP_FONT_H__
#define GLOP_FONT_H__

// Includes
#include "Base.h"
#include "BinaryFileManager.h"
#include "Color.h"
#include "LightSet.h"
#include <map>
using namespace std;

// Class declarations
class DisplayLists;
class FontBitmap;
class TextRenderer;
class Texture;

// Constants
const int kNumFontCharacters = 128;
const unsigned int kFontNormal = 0;
const unsigned int kFontBold = 1;      // Font flags
const unsigned int kFontItalics = 2;
const unsigned int kFontUnderline = 4;

// FontOutline class definition
class FontOutline {
 public:
  // Loads a TrueType font from disk. Returns 0 on failure.
  static FontOutline *Load(BinaryFileReader reader);
  ~FontOutline() {ASSERT(bitmaps_.size() == 0);}

  // Allocate or free a FontBitmap for the given size and flags. Note a user does not delete
  // FontBitmaps. Instead, he calls FontOutline::FreeRef.
  FontBitmap *AddRef(int size, unsigned int flags);
  void FreeRef(int size, unsigned int flags);

 private:
  FontOutline(unsigned char *data, void *face);
  map<pair<int, unsigned int>, FontBitmap*> bitmaps_;
  unsigned char *data_;
  void *face_;
  DISALLOW_EVIL_CONSTRUCTORS(FontOutline);
};

// FontBitmap class definition
class FontBitmap {
 public:
  // Character metrics. If a Font were to use a TrueType font as is, it would use these metrics.
  // However, if it embellishes the font in it's own way, it might adjust these values.
  //  (x1,y1)-(x2,y2): If (0,0) is the "start" of a character on the base-line, these give the
  //                   coordinates where the character bitmap should be rendered to.
  //  dx: When a character is rendered, this is how much the position should move forward along
  //      the baseline before the next character is rendered. Note this may be both larger or
  //      smaller than x2 (e.g. ' ' or perhaps 'T').
  //  ascent/descent: The largest distance a character goes above/below the base-line.
  int GetX1(char ch) const {return char_x1_[ch];}
  int GetY1(char ch) const {return char_y1_[ch];}
  int GetX2(char ch) const {return char_x2_[ch];}
  int GetY2(char ch) const {return char_y2_[ch];}
  int GetDx(char ch) const {return char_dx_[ch];}
  int GetAscent() const {return ascent_;}
  int GetDescent() const {return descent_;}

  // A FontBitmap contains all characters within a single large bitmap. This returns the texture
  // coordinates that should be used for rendering a single character from this bitmap. (tu1,tv1)
  // corresponds to the top-left corner, and (tu2,tv2) corresponds to the bottom-right corner.
  void GetTexCoords(char ch, float *tu1, float *tv1, float *tu2, float *tv2) const;

  // If this font were to be underlined, the underline should begin at GetUnderlineStart pixels
  // below the base-line, and have total thickness GetUnderlineHeight.
  int GetUnderlineStart() const {return ul_start_;}
  int GetUnderlineHeight() const {return ul_height_;}

 private:
   // FontOutline interface
  friend class FontOutline;
  FontBitmap(unsigned char **bitmaps, int *x1, int *y1, int *x2, int *y2, int *dx);
  ~FontBitmap();
  int ref_count_;

  // TextRenderer interface
  friend class TextRenderer;
  Texture *texture_;

  // Data
  unsigned char *data_;                   // The bitmap's raw data
  int width_, height_;
  int char_bitmap_x_[kNumFontCharacters], // The position of each character in the bitmap
      char_bitmap_y_[kNumFontCharacters];
  int char_x1_[kNumFontCharacters], char_y1_[kNumFontCharacters],  // See GetX1() etc.
      char_x2_[kNumFontCharacters], char_y2_[kNumFontCharacters];
  int char_dx_[kNumFontCharacters];
  int descent_, ascent_;
  int ul_start_, ul_height_;
  DISALLOW_EVIL_CONSTRUCTORS(FontBitmap);
};

// Font class definition
class Font {
 public:
  // Font creation, deletion
  static Font *Load(BinaryFileReader reader);
  Font(FontOutline *outline): outline_(outline), is_outline_owned_(false) {}
  virtual ~Font();

  // TextRenderer creation, deletion. Note a user does not delete TextRenderers. Instead, he calls
  // Font::FreeRef or TextRenderer::FreeRef.
  TextRenderer *AddRef(int size, const Color &color, unsigned int flags);
  void FreeRef(int size, const Color &color, unsigned int flags);

 protected:
  Font(FontOutline *outline, bool is_outline_owned)
  : outline_(outline), is_outline_owned_(is_outline_owned) {}
  
  // Character metrics used by the TextRenderer. See FontBitmap. By default, these merely return
  // the answers directly as given by a FontBitmap. However, this behavior can be changed if the
  // Font actually uses more/less space for certain characters.
  friend class TextRenderer;
  virtual int GetX1(const FontBitmap *bitmap, char ch) const {return bitmap->GetX1(ch);}
  virtual int GetX2(const FontBitmap *bitmap, char ch) const {return bitmap->GetX2(ch);}
  virtual int GetDx(const FontBitmap *bitmap, char ch) const {return bitmap->GetDx(ch);}
  virtual int GetAscent(const FontBitmap *bitmap) const {return bitmap->GetAscent();}
  virtual int GetDescent(const FontBitmap *bitmap) const {return bitmap->GetDescent();}

  // Rendering utilities as used (indirectly) by the TextRenderer. The actual character renders
  // will be packed into DisplayLists and then called from there.
  //  RenderChar: Assuming the cursor is positioned with (0,0) being on the base-line at the start
  //              of a character, this renders that character. When the function is called,
  //              bitmap_texture and color will all be set, and blend is enabled. The function can
  //              change this, but it should reset them when done.
  //  RenderUnderline: Assuming text len pixels long is printed with (x,y) being the beginning of
  //                   its baseline, this renders the underline for that text. color, no blending,
  //                   and no texture will be activated when the function begins. Again, these
  //                   properties should be reset if they are changed.
  friend class TextRendererDisplayLists;
  virtual void RenderChar(const FontBitmap *bitmap, const Texture *bitmap_texture,
                          const Color &color, char ch) const;
  virtual void RenderUnderline(const FontBitmap *bitmap, int x, int y, int len,
                               const Color &color) const;

 private:
  map<pair<pair<int, Color>, unsigned int>, TextRenderer*> renderers_;
  FontOutline *outline_;
  bool is_outline_owned_;
  DISALLOW_EVIL_CONSTRUCTORS(Font);
};

// TextRenderer class definition
class TextRenderer {
 public:
  // Frees a reference to the given renderer.
  static void FreeRef(TextRenderer *renderer);

  // Renders the given string of text with (x,y) the coordinates of the baseline start. Note that
  // the text can extend in all four directions from text. The exact boundaries are as follows:
  //  left = x + GetX1(text[0]);
  //  top = y - GetAscent();
  //  width = GetTextWidth("text");
  //  height = GetFullTextHeight();
  // PrintUl is identical, except here (x,y) specifies the top-left corner of the text.
  // When complete, either function guarantees texturing/blending are disabled.
  void Print(int x, int y, const string &text) const;
  void PrintUl(int x, int y, const string &text) const {
    if (text.size() > 0) Print(x - GetX1(text[0]), y + GetAscent(), text);
  }

  // Font metrics. In additions to the basic character metrics discussed in FontOutline, there are
  // several derived metrics.
  //  FullHeight: The total number of pixels that need to be reserved for a line of text to ensure
  //              no overlap. It is exactly Ascent + Descent + 1.
  //  CharWidth: The number of pixels across that "belong" to a single character. In other words,
  //             if "Test" is printed at (0,0), coordinates
  //               GetX1('T') -> GetX1('T') + GetCharWidth('T', true, false)
  //             belong to 'T'. Then, the next GetCharWidth('e', false, false) belong to 'e', and
  //             so on. Note that characters may overlap horizontally (e.g. italics), so
  //             GetCharWidth is not identical to GetX2() - GetX1() + 1.
  //  TextWidth: The total width taken up by a string of text. This is precisely a sum of
  //             CharWidths. is_first_text and is_last_text specifies whether we should ever set
  //             is_first_char and is_last_char when calling GetCharWidth.
  int GetX1(char ch) const {return font_->GetX1(bitmap_, ch);}
  int GetX2(char ch) const {return font_->GetX2(bitmap_, ch);}
  int GetDx(char ch) const {return font_->GetDx(bitmap_, ch);}
  int GetAscent() const {return font_->GetAscent(bitmap_);}
  int GetDescent() const {return font_->GetDescent(bitmap_);}
  int GetFullHeight() const {return GetAscent()+GetDescent()+1;}
  int GetCharWidth(char ch, bool is_first_char, bool is_last_char) const;
  int GetTextWidth(const string &text, bool is_first_text = true, bool is_last_text = true) const;
  
 private:
  // Font interface
  friend class Font;
  TextRenderer(Font *font, FontBitmap *bitmap, int size, const Color &color, unsigned int flags);
  ~TextRenderer();
  int ref_count_;

  DisplayLists *display_lists_;
  Font *font_;
  FontBitmap *bitmap_;
  int size_;
  Color color_;
  unsigned int flags_;
  DISALLOW_EVIL_CONSTRUCTORS(TextRenderer);
};

// ShadowFont class definition. This is an example override of Font that displays each character
// twice - once as a slightly offset "shadow" of the original.
class ShadowFont: public Font {
 public:
  // Constructors:
  //  shadow_dx, shadow_dy: The location of the shadow with respect to the main character. All
  //                        values are fractions of the font Ascent.
  //  shadow_color: The color the shadow is rendered in.
  static ShadowFont *Load(BinaryFileReader reader, float shadow_dx = 0, float shadow_dy = -0.06f,
                          const Color &shadow_color = kBlack);
  ShadowFont(FontOutline *outline, float shadow_dx = 0, float shadow_dy = -0.06f,
             const Color &shadow_color = kBlack)
  : Font(outline), shadow_dx_(shadow_dx), shadow_dy_(shadow_dy), shadow_color_(shadow_color) {}

 protected:
  ShadowFont(FontOutline *outline, bool is_outline_owned, float shadow_dx, float shadow_dy,
             const Color &shadow_color)
  : Font(outline, is_outline_owned), shadow_dx_(shadow_dx), shadow_dy_(shadow_dy),
    shadow_color_(shadow_color) {}

  // Overrides
  int GetX1(const FontBitmap *bitmap, char ch) const {
    return min(GetShadowDx(bitmap), 0) + Font::GetX1(bitmap, ch);
  }
  int GetX2(const FontBitmap *bitmap, char ch) const {
    return max(GetShadowDx(bitmap), 0) + Font::GetX2(bitmap, ch);
  }
  int GetAscent(const FontBitmap *bitmap) const {
    return max(-GetShadowDy(bitmap), 0) + Font::GetAscent(bitmap);
  }
  int GetDescent(const FontBitmap *bitmap) const {
    return max(GetShadowDy(bitmap), 0) + Font::GetDescent(bitmap);
  }
  void RenderChar(const FontBitmap *bitmap, const Texture *bitmap_texture, const Color &color,
                  char ch) const;
  void RenderUnderline(const FontBitmap *bitmap, int x, int y, int len, const Color &color) const;

 private:
  int GetShadowDx(const FontBitmap *bitmap) const;
  int GetShadowDy(const FontBitmap *bitmap) const;
  float shadow_dx_, shadow_dy_;
  Color shadow_color_;
  DISALLOW_EVIL_CONSTRUCTORS(ShadowFont);
};

#endif // GLOP_FONT_H__
