// A raw image, stored in either RGB (24 bit) or RGBA (32 bit) format. The image is made
// compatible with OpenGl so that it can be used as a texture (i.e., the width and height are
// increased to powers of 2), but no texture information is directly stored here. See
// System::LoadTexture for that functionality.
//
// The main function of this class is loading images from data files. Currently, Bmp, Gif and Jpg
// files are supported.

#ifndef GLOP_IMAGE_H__
#define GLOP_IMAGE_H__

// Includes
#include "Base.h"
#include "BinaryFileManager.h"

// Class declarations
class BinaryFileReader;
class Color;

// Texture class definition
class Image {
 public:
  // Load an image from a file.
  // Format 1: The image is loaded with no changes. It is stored in 32 bits only if the original
  //           image is 32 bit, or if force_alpha is true.
  // Format 2: The image is loaded in 32 bit mode, and then any pixels that are within a distance
  //           of bg_tolerance from bg_color are made transparent. Formally: each pixel is taken
  //           as (r,g,b) with r,g,b between 0 and 255. It's distance from bg_color is the sum of
  //           the red difference, the green difference, and the blue difference.
  // On error, 0 is returned.
  static Image *Load(BinaryFileReader reader, bool force_alpha = false);
  static Image *Load(BinaryFileReader reader, const Color &bg_color, int bg_tolerance);
  static Image *ScaledImage(const Image *image, int new_width, int new_height);

  // Destructor. Should only be used by the image creator, so it should not be used if this is
  // created as a system texture.
  ~Image() {delete[] pixels_;}

  // Internal metrics:
  //   Bpp - Bits per pixel (24 or 32).
  //   InternalWidth, InternalHeight - The size of the image after increasing to a power of 2.
  //   Width, Height - The size of the image before increasing to a power of 2.
  int GetBpp() const {return bpp_;}
  int GetBytesPerRow() const {return internal_width_ * bpp_ / 8;}
  int GetInternalWidth() const {return internal_width_;}
  int GetInternalHeight() const {return internal_height_;}
  int GetWidth() const {return width_;}
  int GetHeight() const {return height_;}

  // Raw pixel data, stored as RGB or RGBA (distinguishable via GetBpp).
  const unsigned char *GetPixels() const {return pixels_;}
  unsigned char *GetPixels() {return pixels_;}

 private:
  // Image initialization
  static unsigned int NextPow2(unsigned int n);
  void SmoothTransparentColors();
  static bool IsBmp(BinaryFileReader reader);
  static Image *LoadBmp(BinaryFileReader reader, bool force_alpha);
  static bool IsGif(BinaryFileReader reader);
  static Image *LoadGif(BinaryFileReader reader, bool force_alpha);
  static bool IsJpg(BinaryFileReader reader);
  static Image *LoadJpg(BinaryFileReader reader, bool force_alpha);
  Image(unsigned char *data, int width, int height, int bpp, bool force_alpha);

  unsigned char *pixels_;
  int width_, height_;
  int internal_width_, internal_height_;
  int bpp_;
  DISALLOW_EVIL_CONSTRUCTORS(Image);
};

#endif // IMAGE_H__
