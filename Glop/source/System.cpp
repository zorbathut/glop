// Includes
#include "System.h"
#include "GlopWindow.h"
#include "Image.h"
#include "Input.h"
#include "OpenGl.h"
#include "Os.h"
#include "third_party/freetype/ftglyph.h"
#include <algorithm>

// Globals
System *gSystem = 0;

// Main function. Registers an at-exit clause, and then transfers to GlopMain. This is called by
// the real main function contained in Os___.cpp.
extern int GlopMain(int, char**);
int GlopInternalMain(int argc, char **argv) {
  gSystem = new System();
  atexit(System::ShutDown);
  return GlopMain(argc, argv);
}

// Internal logic - see System.h.
int System::Think() {
  const int kFpsRecordingDelay = 50; // Number of milliseconds between fps updates

  // Calculate the current time, and sleep until we have spent an appropriate amount of time
  // since the last call to Think.
  int ticks = Os::GetTime(), ticks_target = old_time_ + (max_fps_ == 0? 0 : 1000 / max_fps_);
  while (ticks < ticks_target) {
    Os::Sleep(ticks_target - ticks);
    ticks = Os::GetTime();
  }
  int dt = ticks - old_time_;
  old_time_ = ticks;

  // Calculate the current frame rate
  int last_fps_index = (fps_array_index_ + kFpsHistorySize - 1) % kFpsHistorySize;
  if ((fps_array_index_ == 0 && !fps_history_filled_) ||
      ticks >= fps_time_history_[last_fps_index] + kFpsRecordingDelay) {
    fps_frame_history_[fps_array_index_] = frame_count_;
    fps_time_history_[fps_array_index_] = ticks;
    int oldest_fps_index = (fps_history_filled_? (fps_array_index_ + 1) % kFpsHistorySize: 0);
    fps_ = (fps_frame_history_[fps_array_index_] - fps_frame_history_[oldest_fps_index]) * 1000.0f /
           (fps_time_history_[fps_array_index_] - fps_time_history_[oldest_fps_index]);
    fps_array_index_ = (fps_array_index_ + 1) % kFpsHistorySize;
    fps_history_filled_ |= (fps_array_index_ == 0);
  }

  // Free all fonts with 0 reference count. We do this here instead of in ReleaseFontRef so that
  // releasing all font references for a given font and then adding new ones to the same font is
  // fast. This could easily happen if we cleared all frames from a screen and then replaced them.
  for (LightSetId id = fonts_.GetFirstId(); id != 0; id = fonts_.GetNextId(id))
    if (fonts_[id].ref_count == 0) {
      if (fonts_[id].gl_id != 0)            // This would only fail if the window is not created,
        GlUnregisterFont(id);               //  but better safe than sorry.     
      delete[] fonts_[id].image;
      id = fonts_.RemoveItem(id);
    }

  // Perform all Os and window logic
  Os::Think();
  window_->Think(dt);

  // Update our frame and time counts
  frame_count_++;
  return dt;
}

// Time-keeping
// ============

int System::GetTime() {
  return Os::GetTime() - start_time_;
}

void System::Sleep(int t) {
  Os::Sleep(t);
}

// Windowing
// =========

vector<pair<int, int> > System::GetFullScreenModes(int min_width, int min_height) {
  vector<pair<int, int> > all_modes = Os::GetFullScreenModes(), good_modes;
  for (int i = 0; i < (int)all_modes.size(); i++)
    if (all_modes[i].first >= min_width && all_modes[i].second >= min_height)
      good_modes.push_back(all_modes[i]);
  return good_modes;
}

// Texture loading
// ===============

LightSetId System::LoadTexture(BinaryFileReader reader, bool force_alpha,
                               TextureFilter mag_filter, TextureFilter min_filter) {
  Image *image = Image::Load(reader, force_alpha);
  if (image != 0)
    return LoadTexture(image, mag_filter, min_filter);
  else
    return 0;
}

LightSetId System::LoadTexture(BinaryFileReader reader, const Color &bg_color, int bg_tolerance,
                               TextureFilter mag_filter, TextureFilter min_filter) {
  Image *image = Image::Load(reader, bg_color, bg_tolerance);
  if (image != 0)
    return LoadTexture(image, mag_filter, min_filter);
  else
    return 0;
}

// Stores all texture data, and then registers it with OpenGl if the window is actually created.
LightSetId System::LoadTexture(Image *image, TextureFilter mag_filter,
                               TextureFilter min_filter) {
  ASSERT(image != 0);
  Texture texture;
  texture.image = image;
  texture.mag_filter = mag_filter;
  texture.min_filter = min_filter;
  texture.gl_id = 0;
  LightSetId result = textures_.InsertItem(texture);
  if (window_->IsCreated())
    GlRegisterTexture(result);
  return result;
}

// Texture utilities
// =================

void System::ReleaseTexture(LightSetId id) {
  GlUnregisterTexture(id);
  delete textures_[id].image;
  textures_.RemoveItem(id);
}

void System::ReleaseAllTextures() {
  while (textures_.GetSize() > 0)
    ReleaseTexture(textures_.GetFirstId());
}

// Font utilities
// ==============

LightSetId System::LoadFontOutline(BinaryFileReader reader) {
  // Initialize FreeType if it is not already initialized
  if (free_type_library_== 0) {
    if (FT_Init_FreeType((FT_Library*)&free_type_library_))
      return 0;
  }
  
  // Read the file data
  if (!reader.IsOpen())
    return 0;
  int data_length = reader.GetLength();
  unsigned char *data = new unsigned char[data_length];
  reader.ReadChars(data_length, data);

  // Create the font face
  FT_Face face;
  if (FT_New_Memory_Face((FT_Library)free_type_library_, data, data_length, 0, &face)) {
    delete[] data;
    return 0;
  }

  // Store the data
  FontOutline outline;
  outline.face = (void*)face;
  outline.data = data;
  return font_outlines_.InsertItem(outline);
}

void System::ReleaseFontOutline(LightSetId id) {
  FT_Done_Face((FT_Face)font_outlines_[id].face);
  delete[] font_outlines_[id].data;
  font_outlines_.RemoveItem(id);
}

void System::ReleaseAllFontOutlines() {
  while (font_outlines_.GetSize())
    ReleaseFontOutline(font_outlines_.GetFirstId());
}

LightSetId System::AddFontRef(LightSetId outline_id, int height) {
  // Check if we can handle this call without creating a new bitmapped font
  if (height < 1 || outline_id == 0)
      return 0;
  for (LightSetId id = fonts_.GetFirstId(); id != 0; id = fonts_.GetNextId(id))
  if (fonts_[id].outline_id == outline_id && fonts_[id].line_height == height) {
    fonts_[id].ref_count++;
    return id;
  }

  // Init
  Font result_font;
  result_font.outline_id = outline_id;
  result_font.ref_count = 1;
  result_font.gl_id = 0;
  result_font.display_list_base = 0;
  bool error = false;               // Has an error occurred?
  int max_y = -10000;               // What are the font vertical extents? Used to find its height
  FT_Face face = (FT_Face)font_outlines_[outline_id].face;
  FT_BitmapGlyph glyphs[kNumFontCharacters];
  memset(glyphs, 0, sizeof(FT_BitmapGlyph)*kNumFontCharacters);
  
  // Set the size for this FreeType font. Note that FreeType measures all distances in 64ths of
  // a pixel. The 96 is essentially a scale - this seems to give a good correspondence between
  // requested height and actual height.
  FT_Set_Char_Size(face, height*64, 0, 96, 0);

  // Load font glyphs and bitmaps for all characters. Also store all metrics that can be
  // determined without looking at the actual pixel data.
  for (int i = kFirstFontCharacter; i <= kLastFontCharacter && !error; i++) {
    // Attempt to convert this glyph into a bitmap
    FT_Glyph glyph;
    if (FT_Load_Glyph(face, FT_Get_Char_Index(face, i), FT_LOAD_DEFAULT) ||
        FT_Get_Glyph(face->glyph, &glyph) ||
        FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1))
      error = true;
    glyphs[i - kFirstFontCharacter] = (FT_BitmapGlyph)glyph;

    // If it worked, store the result
    if (!error) {
      int index = i - kFirstFontCharacter;
      glyphs[index] = (FT_BitmapGlyph)glyph;
      result_font.char_start_x[index] = glyphs[index]->left;
      result_font.char_start_y[index] = height - glyphs[index]->top;
      max_y = max(max_y, (int)result_font.char_start_y[index] + glyphs[index]->bitmap.rows);
      result_font.char_dx[index] = (short)(face->glyph->advance.x / 64);
      result_font.char_w[index] = glyphs[index]->bitmap.width;
      result_font.char_h[index] = glyphs[index]->bitmap.rows;
    }
  }
  result_font.full_height = max_y;

  // If everything worked create the font
  if (!error) {
    unsigned char *data = 0;
    int img_width = 64, img_height = 64; // An estimated size for the bitmap to store all characters
    
    // Do two passes - one to find the size for the main bitmap, and to copy data into the bitmap
    for (int pass = 0; pass < 2; pass++) {
restart:;
      int x = 0, y = 0, row_height = 0;

      // Loop through each character
      for (int i = 0; i < kNumFontCharacters; i++) {
        FT_Bitmap bitmap = glyphs[i]->bitmap;

        // Position this bitmap, and possibly exit early
        if (x + bitmap.width > img_width) {
          x = 0;
          y += row_height;
          row_height = 0;
        }
        if (y + bitmap.rows > img_height || x + bitmap.width > img_width ) {
          img_width *= 2;
          img_height *= 2;
          ASSERT(pass == 0); // Our size should already be set by the second pass
          goto restart;
        }

        // Place this character
        if (pass == 1) {
          result_font.char_x1[i] = x;
          result_font.char_y1[i] = y;
          for (int ty = 0; ty < bitmap.rows; ty++)
            memcpy(data + (y+ty)*img_width + x, bitmap.buffer + ty*bitmap.width, bitmap.width);
        }

        // Move to the next character
        x += bitmap.width;
        row_height = max(row_height, bitmap.rows);
      }

      // If we successfully finished the first pass, allocate memory. Also, decrease the height as
      // long as everything fits. Now, in the second pass, we will actually data onto our bitmaps.
      if (pass == 0) {
        while (y+row_height <= img_height/2)
            img_height /= 2;
        data = new unsigned char[img_width * img_height];
      }
    }

    // Finish creating the font
    result_font.image = data;
    result_font.image_width = img_width;
    result_font.image_height = img_height;
    result_font.line_height = height;
  }

  // Clear the temporary bitmaps
  for (int i = 0; i < kNumFontCharacters; i++)
  if (glyphs[i] != 0)
    FT_Done_Glyph((FT_Glyph)glyphs[i]);

  // On success, store and register the font with OpenGl. Otherwise, just return 0.
  if (!error) {
    LightSetId result_id = fonts_.InsertItem(result_font);
    if (window_->IsCreated())
      GlRegisterFont(result_id);
    return result_id;
  } else {
    return 0;
  }
}

int System::GetCharWidth(LightSetId font_id, char ch, bool is_first_char, bool is_last_char) const {
  int offset = ch - kFirstFontCharacter, result = fonts_[font_id].char_dx[offset];
  if (is_last_char)
    result = max(result, fonts_[font_id].char_w[offset] + fonts_[font_id].char_start_x[offset]);
  if (is_first_char)
    result -= fonts_[font_id].char_start_x[offset];
  return result;
}

int System::GetTextWidth(LightSetId font_id, const string &text) const {
  int total_width = 0, len = (int)text.size();
  for (int i = 0; i < (int)text.size(); i++)
    total_width += GetCharWidth(font_id, text[i], i == 0, i == len - 1);
  return total_width;
}

// Interface to GlopInternalMain
// =============================

System::System()
: window_(gWindow = new GlopWindow()),
  max_fps_(100),
  frame_count_(0),
  start_time_(Os::GetTime()),
  free_type_library_(0),      // The FreeType library is only initialized when needed
  fps_(0),
  fps_history_filled_(false),
  fps_array_index_(0) {
  old_time_ = start_time_;
}

System::~System() {
  ReleaseAllTextures();
  ReleaseAllFontOutlines();
  delete window_;
  if (free_type_library_ != 0)
    FT_Done_FreeType((FT_Library)free_type_library_);
}

// Interface to GlopWindow
// =======================
//
// For all of our data, we differentiate between being "loaded" and being "registered with
// Open Gl". In the former case, we have preprocessed the data as much as is possible without
// using Open Gl whatsoever. Here, we load data from the file, organize it, and check for errors.
// In the latter case, we actually create the Open Gl object that will be used. Unfortunately,
// when a window is not yet created, or is destroyed (for example by switching into or out of
// full screen mode), the Gl information on the object is lost. To handle this, we supply
// GlUnregisterAll and GlRegisterAll that are called automatically when the window is created or
// deleted.

void System::GlUnregisterAll() {
  for (LightSetId id = textures_.GetFirstId(); id != 0; id = textures_.GetNextId(id))
    GlUnregisterTexture(id);
  for (LightSetId id = fonts_.GetFirstId(); id != 0; id = fonts_.GetNextId(id))
    GlUnregisterFont(id);
}

void System::GlRegisterAll() {
  for (LightSetId id = textures_.GetFirstId(); id != 0; id = textures_.GetNextId(id))
    GlRegisterTexture(id);
  for (LightSetId id = fonts_.GetFirstId(); id != 0; id = fonts_.GetNextId(id))
    GlRegisterFont(id);
}

// Internal texture utilities
// ==========================
// 
// See Interface to GlopWindow above.

void System::GlRegisterTexture(LightSetId id) {
  const int kFilterType[] = {
    GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST,
    GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR};
  ASSERT(!textures_[id].gl_id == 0);

  // Fix the texture settings
  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &textures_[id].gl_id);
  glBindTexture(GL_TEXTURE_2D, textures_[id].gl_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
                              kFilterType[textures_[id].mag_filter]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
                              kFilterType[textures_[id].min_filter]);

  // Create the texture
  int width = textures_[id].image->GetInternalWidth();
  int height = textures_[id].image->GetInternalHeight();
  int format = (textures_[id].image->GetBpp() == 24? GL_RGB : GL_RGBA);
  int internal_format = textures_[id].image->GetBpp() / 8;
  if (textures_[id].mag_filter < 2 && textures_[id].min_filter < 2) {
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, textures_[id].image->GetPixels());
  } else {
    gluBuild2DMipmaps(GL_TEXTURE_2D, internal_format, width, height, format,
                      GL_UNSIGNED_BYTE, textures_[id].image->GetPixels());
  }
}

void System::GlUnregisterTexture(LightSetId id) {
  ASSERT(textures_[id].gl_id != 0);
  glDeleteTextures(1, &textures_[id].gl_id);
  textures_[id].gl_id = 0;
}

// Internal font utilities
// =======================
// 
// See Interface to GlopWindow above.
void System::GlRegisterFont(LightSetId id) {
  ASSERT(fonts_[id].gl_id == 0);

  // Register the underlying texture
  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &fonts_[id].gl_id);
  glBindTexture(GL_TEXTURE_2D, fonts_[id].gl_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  int width = fonts_[id].image_width;
  int height = fonts_[id].image_height;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA,
               GL_UNSIGNED_BYTE, fonts_[id].image);
  
  // Build the display lists
  fonts_[id].display_list_base = glGenLists(kLastFontCharacter+1);
  for (int i = kFirstFontCharacter; i <= kLastFontCharacter; i++) {
    // Get the character dimensions
    int offset = i - kFirstFontCharacter;
    short x1 = fonts_[id].char_x1[offset];
    short y1 = fonts_[id].char_y1[offset];
    short w = fonts_[id].char_w[offset];
    short h = fonts_[id].char_h[offset];
    short start_x = fonts_[id].char_start_x[offset];
    short start_y = fonts_[id].char_start_y[offset];
    short dx = fonts_[id].char_dx[offset];

    // Create a display list to render the character and then move right to the next character
    glNewList(fonts_[id].display_list_base + i, GL_COMPILE);
    glBegin(GL_QUADS);
    glTexCoord2f(float(x1)/width, float(y1)/height);
    glVertex2i(start_x, start_y);
    glTexCoord2f(float(x1+w)/width, float(y1)/height);
    glVertex2i(start_x+w, start_y);
    glTexCoord2f(float(x1+w)/width, float(y1+h)/height);
    glVertex2i(start_x+w, start_y+h);
    glTexCoord2f(float(x1)/width, float(y1+h)/height);
    glVertex2i(start_x, start_y+h);
    glEnd();
    glTranslatef(float(dx), 0, 0);
    glEndList();
  }
}

void System::GlUnregisterFont(LightSetId id) {
  ASSERT(fonts_[id].gl_id != 0);
  glDeleteLists(fonts_[id].display_list_base, kLastFontCharacter+1);
  glDeleteTextures(1, &fonts_[id].gl_id);
  fonts_[id].gl_id = 0;
  fonts_[id].display_list_base = 0;
}
