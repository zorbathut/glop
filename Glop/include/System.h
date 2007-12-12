#ifndef SYSTEM_H__
#define SYSTEM_H__

// Include
#include "Base.h"
#include "BinaryFileManager.h"
#include "LightSet.h"
#include <vector>

// Class declarations
class Color;
class GlopWindow;
class Image;
class System;

// Constants
const int kFirstFontCharacter = 32;  // The character indices that we load with any given font
const int kLastFontCharacter = 126;
const int kNumFontCharacters = kLastFontCharacter - kFirstFontCharacter + 1;

// Globals
extern System *gSystem;

// System class definition
class System {
 public:
  // Startup. Creates gSystem and does all setup we want. ShutDown is done automatically.
  static void Init();

  // Internal logic - Think must be called exactly once per frame. It returns the number of
  // milliseconds that have elapsed since the previous call. During the call to Think, all
  // KeyHandlers receive OnKeyEvent messages, all GlopFrames perform all their logic, and all
  // rendering is performed. See GlopFrameBase.h for a more detailed pipeline.
  int Think();

  // Time-keeping
  // ============

  // Returns the number of milliseconds that have elapsed since the program began.
  int GetTime();

  // Returns the number of times Think has finished executing since the program began.
  int GetFrameCount() const {return frame_count_;}

  // Causes the current thread to give up context and sleep for the given number of milliseconds.
  // During this time, it will cause 0 load on the CPU. If no time is supplied for Sleep, it
  // sleeps for a fraction of the desired frame speed.
  void Sleep(int t);
  void Sleep() {
    Sleep(max_fps_ == 0? 0 : (250 / max_fps_));
  }

  // Returns the current frame rate for the program. This is a running average from data over a
  // fixed time interval.
  float GetFps() {return fps_;}

  // Gets/sets the maximum frame rate for the program. If this is 0, then no artificial frame rate
  // will be set. Otherwise System::Think will automatically sleep to ensure the frame rate does
  // not exceed this value.
  int GetMaxFps() {return max_fps_;}
  void SetMaxFps(int fps) {max_fps_ = fps;}

  // Windowing
  // =========
  
  // Returns all full-screen resolutions (width, height) that are supported on this computer. The
  // resolutions are listed in increasing lexicographical order. min_width and min_height may be
  // set to restrict the list to only contain resolutions with at least a certain size.
  vector<pair<int, int> > GetFullScreenModes(int min_width = 640, int min_height = 480);

  // Returns the main window for this Glop program. This can also be gotten via the gWindow global
  // variable (see GlopWindow.h).
  GlopWindow *window() {return window_;}
  
  // Texture loading
  // ===============
  
  // The LoadTexture functions mirror the Image constructors except that we also augment them with
  // magnification and minimization filters, which OpenGl uses for scaling the textures. In the
  // case of mipmapping, the first filter (L or N) is used to generate the mipmaps themselves, and
  // the second filter is used for scaling the closest mipmap onto the screen.
  //
  // On error, all LoadTexture functions return 0.
  //
  // Note that all texture loading should be done through System instead of manually setting a
  // texture via OpenGl commands. Otherwise, the textures will be lost when a program switches
  // into or out of full screen mode.
  enum TextureFilter {kNearest, kLinear, kMipmapNN, kMipmapLN, kMipmapNL, kMipmapLL};
  LightSetId LoadTexture(BinaryFileReader reader, bool force_alpha = false) {
    return LoadTexture(reader, force_alpha, kLinear, kLinear);
  }
  LightSetId LoadTexture(BinaryFileReader reader, bool force_alpha, TextureFilter mag_filter,
                         TextureFilter min_filter);
  LightSetId LoadTexture(BinaryFileReader reader, const Color &bg_color, int bg_tolerance) {
    return LoadTexture(reader, bg_color, bg_tolerance, kLinear, kLinear);
  }
  LightSetId LoadTexture(BinaryFileReader reader, const Color &bg_color, int bg_tolerance,
                         TextureFilter mag_filter, TextureFilter min_filter);
  LightSetId LoadTexture(Image *image) {return LoadTexture(image, kLinear, kLinear);}
  LightSetId LoadTexture(Image *image, TextureFilter mag_filter, TextureFilter min_filter);

  // Texture utilities
  // =================
  //
  // Note that a mutable Image is never returned since edits to the image will not be reflected
  // within the OpenGl texture. The Gl id of a texture is the id one would use for glBindTexture.
  const Image *GetTextureImage(LightSetId id) const {return textures_[id].image;}
  unsigned int GetTextureGlId(LightSetId id) const {return textures_[id].gl_id;}
  void ReleaseTexture(LightSetId id);
  void ReleaseAllTextures();
  
  // Font utilities
  // ==============

  // Loads a TrueType font. An outline consists of a set of glyphs that describe the font layout
  // for all sizes. Before text can actually be printed, AddFontRef must be called to generate
  // printable bitmaps for the font at a given size.
  LightSetId LoadFontOutline(BinaryFileReader reader);

  // Free one or more TrueType font outlines. All bitmapped fonts generated by AddFontRef for this
  // outline should already be deleted.
  void ReleaseFontOutline(LightSetId id);
  void ReleaseAllFontOutlines();

  // Creates or deletes bitmaps to print an outline font with the given "height". Height here should
  // be similar to the pixel height of the font, but it is not guaranteed to match it exactly.
  // Reference counting is used to ensure that no redundant data is generated. Also, bitmaps will
  // not actually be deleted until the next call to Think, which ensures that ReleaseFontRef
  // followed by AddFontRef will be fast.
  //
  // AddFontRef can fail with bad parameters (e.g. outline_id = 0 or height <= 0), in which case it
  // will return 0.
  LightSetId AddFontRef(LightSetId outline_id, int height);
  void ReleaseFontRef(LightSetId font_id) {fonts_[font_id].ref_count--;}

  // Font metrics.
  //  - GetCharX1 and GetCharX1 return the leftmost and rightmost pixel location for the given
  //    character relative to its "position". GetCharX1 can be negative.
  //  - GetCharDx returns hows far right we move after printing the given character.
  //  - GetCharWidth returns the pixel "width" of a single character - namely how many pixels across
  //    are reserved for it. Since characters can essentially overlap horizontally (e.g. with
  //    italics), this is not the same thing as GetCharX2 - GetCharX1 + 1. The result is
  //    calculated differently for the first and last characters in a line of text, where we need to
  //    ensure we capture overhang outside the normal reserved area.
  //  - GetTextWidth reserves the amount of horizontal space reserved for a line of text. This is
  //    equivalent to summing GetCharWidth with is_first_char and is_last_char set for the first
  //    and last character respectively.
  int GetCharX1(LightSetId font_id, char ch) const {
    return fonts_[font_id].char_start_x[ch - kFirstFontCharacter];
  }
  int GetCharX2(LightSetId font_id, char ch) const {
    return fonts_[font_id].char_start_x[ch - kFirstFontCharacter] +
      fonts_[font_id].char_w[ch - kFirstFontCharacter] - 1;
  }
  int GetCharDx(LightSetId font_id, char ch) const {
    return fonts_[font_id].char_dx[ch - kFirstFontCharacter];
  }
  int GetCharWidth(LightSetId font_id, char ch, bool is_first_char, bool is_last_char) const;
  int GetTextWidth(LightSetId font_id, const string &text) const;

  // A convenience function that returns the width of a line of text in the given font. This is
  // equivalent to adding GetCharWidth for each character with count_offset true for the first
  // character, and false otherwise.
  //int GetTextWidth(LightSetId font_id, const string &text) const;

  // Returns the height of a line of text in the given font. If count_overhang == true, the height
  // includes pixels that might be generated below the base-line (e.g. the bottom part of a 'p').
  // Otherwise, these pixels are ignored.
  // GetFontHeight(false) is guaranteed to be the height given to AddFontRef.
  int GetFontHeight(LightSetId font_id, bool count_overhang) const {
    return (count_overhang? fonts_[font_id].full_height : fonts_[font_id].line_height);
  }

  // Returns the Gl id corresponding to the texture containing all characters in this font.
  unsigned int GetFontGlId(LightSetId id) const {return fonts_[id].gl_id;}

  // Returns the first Gl id in a contiguous group of display lists, where GetFontDisplayList + ch
  // gives a display list that draws the character ch and then moves right to where the next
  // character would be drawn.
  unsigned int GetFontDisplayList(LightSetId id) const {return fonts_[id].display_list_base;}

 private:
  System();
  static void ShutDown() {delete gSystem;}
  ~System();
  
  // Interface to GlopWindow
  friend class GlopWindow;
  void GlUnregisterAll();
  void GlRegisterAll();

  // General data
  GlopWindow *window_;
  int max_fps_;
  int frame_count_;
  int start_time_, old_time_;  // Os::Time as of program start and as of the last call to Think
  void *free_type_library_;    // Internal handle to the FreeType library, used for text

  // FPS data
  static const int kFpsHistorySize = 20;
  float fps_;                              // Our frame rate as of the last calculation
  bool fps_history_filled_;                // Have we made at least kFpsHistorySize measurements?
  int fps_array_index_;                    // What's the next entry in the history to fill?
  int fps_frame_history_[kFpsHistorySize]; // Frame and time measurements. Measurements are made at
  int fps_time_history_[kFpsHistorySize];  //  time intervals, not after a fixed number of frames,
                                           //  which is why fps_frame_history is needed.
  
  // Internal texture data and utilities
  struct Texture {
    Image *image;
    TextureFilter min_filter, mag_filter;
    unsigned int gl_id;
  };
  LightSet<Texture> textures_;
  void GlRegisterTexture(LightSetId id);
  void GlUnregisterTexture(LightSetId id);

  // Internal font data and utilities
  struct FontOutline {
    void *face;
    unsigned char *data;
  };
  LightSet<FontOutline> font_outlines_;
  struct Font {
    LightSetId outline_id;
    int ref_count;
    unsigned char *image;                   // A single bitmap containing inside it all bitmaps for
                                            //  this font (stored in Alpha-only format)
    int image_width, image_height;          // The size of the font bitmap above
    short char_x1[kNumFontCharacters],      // Coordinates of each character's top-left corner in
          char_y1[kNumFontCharacters];      //  the font image bitmap
    short char_w[kNumFontCharacters],       // Size of each character's bitmap
          char_h[kNumFontCharacters];
    short char_start_x[kNumFontCharacters], // The coordinates where (0,0) on each character bitmap
          char_start_y[kNumFontCharacters]; //  should be rendered to
    short char_dx[kNumFontCharacters];      // The amount the x position should be increased by
                                            //  after rendering each character
    int full_height, line_height;           // See GetFontHeight
    unsigned int gl_id;                     // See GetFontGlId
    unsigned int display_list_base;         // See GetFontDisplayList
  };
  void GlRegisterFont(LightSetId id);
  void GlUnregisterFont(LightSetId id);
  LightSet<Font> fonts_;

  DISALLOW_EVIL_CONSTRUCTORS(System);
};

#endif // SYSTEM_H__
