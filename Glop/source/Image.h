// An Image is raw image data stored in one of the following formats:
//  - Alpha (bpp == 8)
//  - Luminance, alpha (bpp == 16)
//  - Red, green, blue (bpp == 24)
//  - Red, green, blue, alpha (bpp == 32)
// It is designed to be very easily used as an OpenGL texture. Thus, the actually data is stored
// with width and height automatically increased to powers of 2.
//
// Nonetheless, an Image is fundamentally just raw data. To actually render it, a Texture object
// must be cerated (see OpenGl.h).
//
// The main function of this class is to provide support for loading an Image from a file.

#ifndef GLOP_IMAGE_H__
#define GLOP_IMAGE_H__

// Includes
#include "Base.h"
#include "Stream.h"

// Class declarations
class Color;

// Image class definition
class Image {
 public:
  // Image constructors:
  // Format 1: Space for the image is allocated, but it is left empty.
  // Format 2: The image is constructed from the given data, representing a raw bitmap with the
  //           given width and height. Thus, data should be exactly width*height*bpp bytes long (as
  //           opposed to the Image itself, which uses power of 2 sizes).
  // Format 3: The image is loaded from a file with no changes. 0 is returned on read error.
  // Format 4: A utility to add alpha to RGB images. The image is loaded in 32 bit mode, and then
  //           any pixels that are within a distance of bg_tolerance from bg_color are made
  //           transparent. Distance = rdiff + gdiff + bdiff.
  // Format 5: The image is a rescaled version of the given image.
  Image(int width, int height, int bpp);
  Image(unsigned char *data, int width, int height, int bpp);
  static Image *Load(InputStream input);
  static Image *Load(InputStream input, const Color &bg_color, int bg_tolerance);
  static Image *AdjustedImage(const Image *image, int new_width, int new_height, int new_bpp);
  ~Image() {delete[] data_;}

  // Internal metrics:
  //   Bpp - Bits per pixel (24 or 32).
  //   InternalWidth, InternalHeight - The size of the image after increasing to dimensions to
  //                                   powers of 2.
  //   Width, Height - The original size of the image.
  int GetBpp() const {return bpp_;}
  int GetBytesPerRow() const {return internal_width_ * bpp_ / 8;}
  int GetInternalWidth() const {return internal_width_;}
  int GetInternalHeight() const {return internal_height_;}
  int GetWidth() const {return width_;}
  int GetHeight() const {return height_;}

  // Raw pixel data, stored in A, LA, RGB, or RGBA format.
  const unsigned char *Get() const {return data_;}
  unsigned char *Get() {return data_;}
  const unsigned char *Get(int x, int y) const {return data_ + (y*internal_width_ + x)*(bpp_/8);}
  unsigned char *Get(int x, int y) {return data_ + (y*internal_width_ + x)*(bpp_/8);}

 private:
  // Image initialization
  static unsigned int NextPow2(unsigned int n);
  void SmoothTransparentColors();
  static bool IsBmp(InputStream input);
  static Image *LoadBmp(InputStream input);
  static bool IsGif(InputStream input);
  static Image *LoadGif(InputStream input);
  static bool IsJpg(InputStream input);
  static Image *LoadJpg(InputStream input);
  static bool IsTga(InputStream input);
  static Image *LoadTga(InputStream input);

  unsigned char *data_;
  int width_, height_;
  int internal_width_, internal_height_;
  int bpp_;
  DISALLOW_EVIL_CONSTRUCTORS(Image);
};

#endif // IMAGE_H__
