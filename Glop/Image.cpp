#include "Color.h"
#include "Image.h"
#include "Stream.h"
extern "C" {
#include "jpeglib/jpeglib.h"
}
#include <cstdlib>
#include <cstring>
#include <png.h>
#include <setjmp.h>  // For LoadJpg error handling

#include <vector>

using namespace std;

// Constants
const int kMaxImageWidth = 65536;
const int kMaxImageHeight = 65536;

// Constructors
Image::Image(int width, int height, int bpp)
: data_(0), width_(width), height_(height), bpp_(bpp) {
  ASSERT(bpp_ > 0 && bpp_ <= 32 && bpp%8 == 0);
  internal_width_ = NextPow2(width);
  internal_height_ = NextPow2(height);
  data_ = new unsigned char[internal_height_ * internal_width_ * bpp_ / 8];
  memset(data_, 0, internal_height_ * internal_width_ * bpp_ / 8);
}

Image::Image(unsigned char *data, int width, int height, int bpp)
: data_(0), width_(width), height_(height), bpp_(bpp) {
  // Store the values
  ASSERT(bpp_ > 0 && bpp_ <= 32 && bpp%8 == 0);
  internal_width_ = NextPow2(width);
  internal_height_ = NextPow2(height);
  int row_size = internal_width_ * bpp_ / 8, old_row_size = width_ * bpp_ / 8;
  data_ = new unsigned char[internal_height_ * row_size];

  // Copy the data into the new location. If width and height are not powers of 2, we replicate
  // the right and down boundaries so GL_LINEAR filter will look nice. All other pixels are black
  // and transparent.
  if (width < internal_width_) {
    for (int y = 0; y < height_; y++) {
      memcpy(data_ + y*row_size, data + y*old_row_size, old_row_size);
      memset(data_ + y*row_size + width_*bpp_/8, 0, row_size - old_row_size);
      memcpy(data_ + y*row_size + width_*bpp_/8, data_ + y*row_size + (width_-1)*bpp_/8, bpp_/8);
    }
  } else {
    memcpy(data_, data, height * row_size);
  }
  if (height_ < internal_height_) {
    memset(data_ + height_*row_size, 0, (internal_height_-height)*row_size);
    memcpy(data_ + height_*row_size, data_ + (height_-1)*row_size, row_size);
  }
}

Image *Image::Load(InputStream input) {
  if (!input.IsValid())
    return 0;
  if (IsBmp(input))
    return LoadBmp(input);
  if (IsGif(input))
    return LoadGif(input);
  if (IsJpg(input))
    return LoadJpg(input);
  if (IsTga(input))
    return LoadTga(input);
  if (IsPng(input))
    return LoadPng(input);
  return 0;
}

Image *Image::Load(InputStream input, const Color &bg_color, int bg_tolerance) {
  // First load the image normally (but in 32 bit format)
  Image *result = Load(input);
  if (result == 0)
    return 0;

  // Convert to 32 bpp
  int x, y, w = result->GetWidth(), h = result->GetHeight();
  ASSERT(result->GetBpp() == 24);
  Image *temp = Image::AdjustedImage(result, w, h, 32);
  delete result;
  result = temp;

  // Set the alpha channel based on the background
  int target[] = {(int)(bg_color[0]*255), (int)(bg_color[1]*255), (int)(bg_color[2]*255)};
  for (y = 0; y < h; y++)
  for (x = 0; x < w; x++) {
    unsigned char *pixel = result->Get(x, y);
    int d = abs(pixel[0] - target[0]) + abs(pixel[1] - target[1]) + abs(pixel[2] - target[2]);
    if (d <= bg_tolerance)
      pixel[3] = 0;
    else
      pixel[3] = 255;
  }
  result->SmoothTransparentColors();
  return result;
}

Image *Image::AdjustedImage(const Image *image, int new_width, int new_height, int new_bpp) {
  int bpp = image->GetBpp();
  unsigned char *data = (unsigned char*)malloc(new_width * new_height * new_bpp);
  for (int y = 0; y < new_height; y++)
  for (int x = 0; x < new_width; x++) {
    // Extract the source pixel
    int x2 = x * image->GetWidth() / new_width;
    int y2 = y * image->GetHeight() / new_height;
    const unsigned char *src = image->Get(x2, y2);
    unsigned char r, g, b, a;
    if (bpp == 8) {
      r = g = b = 255;
      a = src[0];
    } else if (bpp == 16) {
      r = g = b = src[0];
      a = src[1];
    } else {
      r = src[0];
      g = src[1];
      b = src[2];
      a = (bpp == 32? src[3] : 255);
    }

    // Write the destination pixel
    unsigned char *dst = data + (new_width*y + x) * (new_bpp / 8);
    if (new_bpp == 8) {
      dst[0] = a;
    } else if (new_bpp == 16) {
      dst[0] = (unsigned char)((int(r) + int(g) + int(b)) / 3);
      dst[1] = a;
    } else {
      dst[0] = r;
      dst[1] = g;
      dst[2] = b;
      if (new_bpp == 32)
        dst[3] = a;
    }
  }
  Image *result = new Image(data, new_width, new_height, new_bpp);
  if (bpp == 24 && new_bpp != 24)
    result->SmoothTransparentColors();
  free(data);
  return result;
}

// Returns the first power of 2 that is at least min(n, 4).
unsigned int Image::NextPow2(unsigned int n) {
  unsigned int temp = 4;
  while (temp < n)
    temp *= 2;
  return temp;
}

// In OpenGl, we often render a combination of two adjacent pixels. Even if a pixel is completely
// transparent, it's color might be used if it is adjacent to a non-transparent pixel. This can
// lead to borders around the image. To prevent that, we use this function to set transparent pixel
// colors to be the average of the non-transparent colors around the pixel.
void Image::SmoothTransparentColors() {
  ASSERT(bpp_ == 16 || bpp_ == 32);
  int w = internal_width_, h = internal_height_, ai = (bpp_ / 8) - 1;
  for (int y = 0; y < h; y++)
  for (int x = 0; x < w; x++) {
    unsigned char *pixel = Get(x, y);
    if (pixel[ai] != 0)
      continue;
    int total[4] = {0, 0, 0};
    for (int y2 = -1; y2 <= 1; y2++)
    for (int x2 = -1; x2 <= 1; x2++) {
      unsigned char *pixel2 = Get((x+x2+w)%w, (y+y2+h)%h);
      for (int i = 0; i < ai; i++)
        total[i] += pixel2[i];
      total[ai]++;
    }
    for (int i = 0; i < ai; i++)
      pixel[i] = (total[ai] == 0? 0 : total[i] / total[ai]);
  }
}

// Bmp loading
// ===========

// Returns whether a input is pointing to a BMP file. The file pointer location is not changed.
bool Image::IsBmp(InputStream input) {
  char tag[2];
  bool is_bmp = (input.LookAheadReadChars(0, 2, tag) == 2 && tag[0] == 'B' && tag[1] == 'M');
  return is_bmp;
}

// Loads a BMP file. Supports 1, 4, 8, 16, 24, and 32 bit uncompressed pixels.
// Adapted from the SDL image library.
Image *Image::LoadBmp(InputStream input) {
  const int kRgb = 0;
  const int kRle8 = 1;
  const int kRle4 = 2;
  unsigned int rmask = 0, gmask = 0, bmask = 0, amask = 0;
  unsigned int rshift = 1, gshift = 1, bshift = 1, ashift = 1;

  // Variables - these must all be defined up here so that we can goto error
  Image *result = 0;                        // The return value
  unsigned char *palette = 0, *pixels = 0;  // Image data (palette is used if bpp <= 8)
  unsigned char *buffer = 0;                // A location to load a line of data into
  int new_bpp = 0;

  // Read the header
  char tag[2];
  int start_pos = input.GetPosition();
  if (input.ReadChars(2, tag) < 2 || tag[0] != 'B' || tag[1] != 'M')
    goto error;
  input.SkipAhead(8);
  int image_start, header_size;
  if (!input.ReadInts(1, &image_start) ||
      !input.ReadInts(1, &header_size))
    goto error;
  int width, height, compression;
  short bpp;
	if (header_size == 12) {
    unsigned short short_width, short_height;
    if (!input.ReadShorts(1, &short_width) ||
        !input.ReadShorts(1, &short_height))
      goto error;
    width = short_width;
    height = short_height;
    input.SkipAhead(2);
    if (!input.ReadShorts(1, &bpp))
      goto error;
    compression = kRgb;
	} else {
    if (!input.ReadInts(1, &width) ||
        !input.ReadInts(1, &height))
      return 0;
    input.SkipAhead(2);
    if (!input.ReadShorts(1, &bpp) ||
        !input.ReadInts(1, &compression))
      return 0;
    input.SkipAhead(20);
	}
  if (bpp != 1 && bpp != 4 && bpp != 8 && bpp != 15 && bpp != 16 && bpp != 24 && bpp != 32)
    return 0;
  if (width < 0 || width > kMaxImageWidth || height < 0 || height > kMaxImageHeight)
    return 0;
  new_bpp = (bpp == 32? 32 : 24);

  // Read the color masks if they exist
	switch (compression) {
		case kRgb:
			// If there are no masks, use defaults
      if (image_start == 14 + header_size) {
				switch (bpp) {
					case 15:
					case 16:
						rmask = 0x7C00;
						gmask = 0x03E0;
						bmask = 0x001F;
						break;
					case 24:
						rmask = 0x00FF0000;
						gmask = 0x0000FF00;
						bmask = 0x000000FF;
						break;
					case 32:
						amask = 0xFF000000;
						rmask = 0x00FF0000;
						gmask = 0x0000FF00;
						bmask = 0x000000FF;
						break;
					default:
						break;
				}
				break;
			}
			// If there are masks or compression is not RGB, read the masks from the file
		default:
      if (bpp == 15 || bpp == 16 || bpp == 32) {
        if (!input.ReadInts(1, &rmask) || !input.ReadInts(1, &gmask) ||
            !input.ReadInts(1, &bmask) || !input.ReadInts(1, &amask))
          return 0;
      }
	}
  while (rmask > 0 && (rshift & rmask) == 0)
    rshift *= 2;
  while (gmask > 0 && (gshift & gmask) == 0)
    gshift *= 2;
  while (bmask > 0 && (bshift & bmask) == 0)
    bshift *= 2;
  while (amask > 0 && (ashift & amask) == 0)
    ashift *= 2;

	// Read the palette if it exists
  if (bpp <= 8) {
    int colors_used = 1 << bpp;
    palette = (unsigned char*)malloc(768);
    input.SkipAhead(14 + header_size - (input.GetPosition() - start_pos));
		for (int i = 0; i < colors_used; i++) {
      if (!input.ReadChars(1, palette+3*i+2) ||
          !input.ReadChars(1, palette+3*i+1) ||
          !input.ReadChars(1, palette+3*i+0))
        goto error;
      if (header_size != 12 && !input.ReadChars(1, tag)) // Dummy character
        goto error;
    }
	}

	// Read the pixel data, and convert it to RGB (or RGBA).
  // Note that the image is stored upside-down.
  input.SkipAhead(image_start - (input.GetPosition() - start_pos));
  pixels = (unsigned char*)malloc(width * height * new_bpp / 8);
  if (compression == kRle4 || compression == kRle8) {
    int x = 0, y = height - 1;
    unsigned char dx, dy;
    unsigned char ch, data, pixel;
    bool is_done = false;
    while (!is_done) {
      if (!input.ReadChars(1, &ch))
        goto error;
      if (ch > 0) {
        // Handle runs (with RLE4, it is a run of period 2 instead of period 1)
        if (!input.ReadChars(1, &data))
          goto error;
        for (int i = 0; i < ch; i++) {
          if (compression == kRle4)
            pixel = (i % 2 == 0? data / 16 : data % 16);
          memcpy(pixels + y*width*3 + (x++)*3, palette + 3*pixel, 3);
        }
		  } else {
        // Handle escape copes
        if (!input.ReadChars(1, &ch))
          goto error;
			  switch (ch) {
			    case 0: // End of line
  				  x = 0;
            y--;
				    break;
			    case 1: // Done
            is_done = true;
				    break;
			    case 2: // Warp
            if (!input.ReadChars(1, &dx) || !input.ReadChars(1, &dy))
              goto error;
            x += dx;
            y -= dy;
            if (x >= width || y < 0)
              goto error;
				    break;
			    default: // Uncompressed data
            if (compression == kRle8) {
              for (int i = 0; i < ch; i++) {
                if (!input.ReadChars(1, &data))
                  goto error;
                memcpy(pixels + y*width*3 + (x++)*3, palette + data*3, 3);
              }
              if (ch % 2 == 1) {
                if (!input.ReadChars(1, &data))
                  goto error;
              }
            } else {
              unsigned char data;
              for (int i = 0; i < ch; i++) {
                if (i % 2 == 0) {
                  if (!input.ReadChars(1, &data))
                    goto error;
                } else {
                  data *= 16;
                }
                unsigned char pixel = data / 16;
                memcpy(pixels + y*width*3 + (x++)*3, palette + pixel, 3);
              }
              if (((ch+1)/2) % 2 == 1) {
                if (!input.ReadChars(1, &data))
                  goto error;
              }
            }
			  }
		  }
	  }
  } else {
    buffer = (unsigned char*)malloc(4 * width);

    // Handle RGB data
    for (int y = height-1; y >= 0; y--) {
      if (bpp <= 8) {
        // Handle palettized RGB data
        int density = 8/bpp;
        int line_length = (width + (8 / bpp) - 1) / (8 / bpp);
        line_length += (4 - line_length % 4) % 4;
        if (input.ReadChars(line_length, buffer) < line_length)
          goto error;
        for (int x = 0; x < width; x++) {
          int offset = x / density;
          int shift = density - 1 - x % density;
          unsigned char data = buffer[offset];
          for (int i = 0; i < shift; i++)
            data /= (1 << bpp);
          memcpy(pixels+3*y*width+3*x, palette+3*(data % (1 << bpp)), 3);
          if (x % (8 / bpp) == 0)
            data++;
        }
      } else {
        // Handle non-palettized RGB data
        if (input.ReadChars(width * bpp / 8, buffer) < width * bpp / 8)
          goto error;
        for (int x = 0; x < width; x++) {
          unsigned int *pixel = (unsigned int*)(buffer + x*(bpp/8));
          *(pixels+(new_bpp/8)*(y*width+x)+0) = (unsigned char)(((*pixel) & rmask) / rshift);
          *(pixels+(new_bpp/8)*(y*width+x)+1) = (unsigned char)(((*pixel) & gmask) / gshift);
          *(pixels+(new_bpp/8)*(y*width+x)+2) = (unsigned char)(((*pixel) & bmask) / bshift);
          if (new_bpp == 32)
            *(pixels+(new_bpp/8)*(y*width+x)+3) = (unsigned char)(((*pixel) & amask) / ashift);
        }
      }
    }
	}

  result = new Image(pixels, width, height, new_bpp);
error:
  if (buffer != 0)
    free(buffer);
  if (palette != 0)
    free(palette);
  if (pixels != 0)
    free(pixels);
  return result;
}

// Gif loading
// ===========

// Returns whether a input is pointing to a GIF file. The file pointer location is not changed.
bool Image::IsGif(InputStream input) {
  unsigned char tag[6];
  bool is_gif = (input.LookAheadReadChars(0, 6, tag) == 6 && memcmp(tag, "GIF", 3) == 0 &&
                 (memcmp(tag+3, "87a", 3) == 0 || (memcmp(tag+3, "89a", 3) == 0)));
  return is_gif;
}

// Gif helper function - loads the given number of bits from the file. File format: byte indicating
// block length, followed by a block of that length. All blocks are logically concatenated until
// a block of length 0 is reached.
static int LoadGifBits(InputStream input, unsigned char *buffer, int num_bits,
                       int *buffer_pos, int *buffer_end) {
  if (*buffer_pos + num_bits >= *buffer_end) {
    if (*buffer_end >= 16)
      buffer[0] = buffer[*buffer_end/8 - 2];
    if (*buffer_end >= 8)
      buffer[1] = buffer[*buffer_end/8 - 1];
    unsigned char block_size;
    if (!input.ReadChars(1, &block_size) || block_size == 0)
      return -1;
    if (input.ReadChars(block_size, buffer + 2) < block_size)
      return -1;
    *buffer_pos = 16 - (*buffer_end - *buffer_pos);
    *buffer_end = 8 * (int(block_size) + 2);
  }
  unsigned int value = 0;
  for (int i = *buffer_pos, j = 0; j < num_bits; i++, j++)
	  value |= (((buffer[i / 8] & (1 << (i % 8))) > 0) << j);
  *buffer_pos += num_bits;
  return value;
}

// Loads a GIF file.
// Adapted from the SDL image library, which is, in turn, taken from XPaint.
Image *Image::LoadGif(InputStream input) {
  const int kMaxLwzBits = 12;
  unsigned char *palette = (unsigned char*)malloc(768), *pixels = 0;
  Image *result = 0;

  // Read the header
  unsigned char block_size;
  unsigned char buffer[260];
  int transparent_index = -1;
  unsigned short width, height, bpp;
  if (input.ReadChars(6, buffer) < 6)
    goto error;
  if (memcmp(buffer, "GIF", 3) != 0)
    goto error;
  input.SkipAhead(4);
  unsigned char data, data2;
  if (!input.ReadChars(1, &data))
    goto error;
  bpp = 1 + (data % 8);
  if (!input.ReadChars(1, &data2))
    goto error;
  transparent_index = data2;
  input.SkipAhead(1);

  // Read the global palette if it exists
  if ((data & 0x80) > 0) {
    if (input.ReadChars(3*(1<<bpp), palette) < 3*(1<<bpp))
      goto error;
  }

  // Read GIF data
  while (1) {
    if (!input.ReadChars(1, &data))
      goto error;
    if (data == '!') {
      // Extension
      if (!input.ReadChars(1, &data))
        goto error;
      if (data == 0xf9) { // Graphic control extension
        if (!input.ReadChars(1, &block_size))
          goto error;
        if (input.ReadChars(block_size, buffer) < block_size)
          goto error;
        if (buffer[0] % 2 == 1)
          transparent_index = buffer[3];
      }
      do {
        if (!input.ReadChars(1, &block_size))
          goto error;
        if (input.ReadChars(block_size, buffer) < block_size)
          goto error;
      } while (block_size > 0);
    } else if (data == ',') {
      // Image start - read the header and the local palette if it exists
      input.SkipAhead(4);
      if (!input.ReadShorts(1, &width) || !input.ReadShorts(1, &height) ||
          !input.ReadChars(1, &data))
        goto error;
      if ((data & 0x80) > 0) {
        bpp = 1 + (data % 8);
        if (input.ReadChars(3*(1<<bpp), palette) < 3*(1<<bpp))
          goto error;
      }
      bool is_interlaced = (data & 0x40) > 0;
      int new_bpp = (transparent_index == -1? 24 : 32);
      unsigned char base_code_size, code_size = 0;
      if (!input.ReadChars(1, &base_code_size) || base_code_size > kMaxLwzBits)
        return 0;
      pixels = (unsigned char*)malloc(width * height * new_bpp / 8);
      
      // GIF uses LZW compression. We store a string table where
      //   S[i] = S[table[0][i]] + table[1][i].
      // This table is initialized to only the base alphabet, but is enlarged implicitly as we go.
      // After reading i:
      //  - If i is in the table, we output S[i], and add S[old] + first char of S[i] to S.
      //  - If i is not in the table, we output S[old] + first char of S[old], and add this to S.
      //  - The size in bits of each index grows until it hits a certain cap, at which point we
      //    reset everything.
      int buffer_pos = 0, buffer_end = 0;
      int x = 0, y = 0, pass = 0;
      int table[2][1 << kMaxLwzBits], stack[1 << (kMaxLwzBits + 1)], stack_pos = -1;
      int first_code = -1, old_code = -1, clear_code = 0, max_code = 0;
      while (y < height) {
        // Read a byte
        int pixel;
        if (stack_pos == -1) {
          while (1) {
            int code = (first_code == -1? clear_code :
                        LoadGifBits(input, buffer, code_size, &buffer_pos, &buffer_end));
            if (code == -1)
              goto error;
            if (code == clear_code) {
              // Initialization
              code_size = base_code_size + 1;
              clear_code = (1 << base_code_size);
              max_code = clear_code + 2;
              stack_pos = -1;
    	        for (int i = 0; i < (1 << kMaxLwzBits); ++i) {
		            table[0][i] = 0;
                table[1][i] = (i < clear_code? i : 0);
	            }
              do {
                first_code = LoadGifBits(input, buffer, code_size, &buffer_pos, &buffer_end);
                if (first_code == -1)
                  goto error;
              } while (first_code == (1 << base_code_size));
              pixel = old_code = first_code;
              break;
            } else if (code == clear_code + 1) // EOF
              goto error;
            int in_code = code;
            if (code >= max_code) {
              stack[++stack_pos] = first_code;
              code = old_code;
            }
            while (code >= clear_code) {
              stack[++stack_pos] = table[1][code];
              code = table[0][code];
            }
            stack[++stack_pos] = first_code = table[1][code];
	          if ((code = max_code) < (1 << kMaxLwzBits)) {
              table[0][code] = old_code;
              table[1][code] = first_code;
              max_code++;
              if (max_code >= (1 << (code_size)) && code_size < kMaxLwzBits)
                code_size++;
            }
            old_code = in_code;
            if (stack_pos >= 0) {
	            pixel = stack[stack_pos--];
              break;
            }
          }
        } else {
          pixel = stack[stack_pos--];
        }

        // Place a pixel
        if (pixel < 0 || pixel > 255)
          goto error;
        memcpy(pixels + (new_bpp/8)*(y*width + x), palette+3*pixel, 3);
        if (transparent_index != -1)
          pixels[(new_bpp/8)*(y*width + x) + 3] = (pixel == transparent_index? 0 : 255);
        x++;
        if (x >= width) {
          x = 0;
          if (is_interlaced) {
            switch (pass) {
              case 0:
              case 1:
                y += 8;
                break;
              case 2:
                y += 4;
              case 3:
                y += 2;
                break;
            }
            if (y >= height) {
              pass++;
              switch (pass) {
                case 1:
                  y = 4;
                  break;
                case 2:
                  y = 2;
                  break;
                case 3:
                  y = 1;
                  break;
              }
            }
          } else {
            y++;
          }
        }
      }
      // Build the image
      result = new Image(pixels, width, height, new_bpp);
      if (new_bpp == 32)
        result->SmoothTransparentColors();
      break;
    } else { // Invalid symbol or EOF before the first image
      break;
    }
  }

error:
  free(palette);
  if (pixels != 0)
    free(pixels);
  return result;
}

// Jpg loading
// ===========

// Returns whether a input is pointing to a JPG file. The file pointer location is not changed.
bool Image::IsJpg(InputStream input) {
  unsigned char tag[3];
  bool is_jpg = (input.LookAheadReadChars(0, 3, tag) == 3 && tag[0] == 0xFF &&
                 tag[1] == 0xD8 && tag[2] == 0xFF);
  return is_jpg;
}

// Jpeg error handling. We use setjmp to jump to a specific location within LoadJpg if jpeglib
// has an error.
struct CustomJpgErrorManager {
	struct jpeg_error_mgr standard_manager;
	jmp_buf jump_location;
};
static void OnJpgErrorExit(j_common_ptr info) {
	CustomJpgErrorManager *error_manager = (CustomJpgErrorManager*)info->err;
	longjmp(error_manager->jump_location, 1);
}
static void OnJpgErrorMessage(j_common_ptr info) {}

// Called by jpeg_read_header before any data is read.
void JpegMemoryInitSource(j_decompress_ptr info) {}

// Fills the buffer when it's emptied. This should not happen for valid files in theory, but I
// think it sometimes does. We just add an end of input marker.
static unsigned char kEndOfInputBuffer[2] = {0xFF, JPEG_EOI};
boolean JpegMemoryFillInputBuffer(j_decompress_ptr info) {
  info->src->next_input_byte = kEndOfInputBuffer;
  info->src->bytes_in_buffer = 2;
  return TRUE;
}

// Skips over data
void JpegMemorySkipInputData(j_decompress_ptr info, long num_bytes) {
  if (num_bytes > 0) {
    while (num_bytes > (long)info->src->bytes_in_buffer) {
      num_bytes -= (long)info->src->bytes_in_buffer;
      JpegMemoryFillInputBuffer(info);
    }
    info->src->next_input_byte += (size_t)num_bytes;
    info->src->bytes_in_buffer -= (size_t)num_bytes;
  }
}

// Called by jpeg_finish_decompress
void JpegMemoryTerminateSource(j_decompress_ptr info) {}

void JpegMemorySource(j_decompress_ptr info, const unsigned char *buffer, int buffer_size) {}

// Loads a JPG file using jpeglib.
Image *Image::LoadJpg(InputStream input) {
  unsigned char *pixels = 0;
  Image *result = 0;
  unsigned char *compressed_data = 0;
  int compressed_data_length;
  jpeg_decompress_struct info;
  jpeg_source_mgr *source_manager = NULL;
	int width = 0;
	int height = 0;
	int bpp = 0;

  // Set up the error handler. If jpeglib runs into an error at any future point, it will
  // execute the block after setjmp(), which will otherwise remain unexecuted.
  CustomJpgErrorManager error_manager;
  info.err = jpeg_std_error(&error_manager.standard_manager);
  error_manager.standard_manager.error_exit = OnJpgErrorExit;
  error_manager.standard_manager.output_message = OnJpgErrorMessage;
  if (setjmp(error_manager.jump_location))
    goto error;

  // Set up the memory source
  jpeg_create_decompress(&info);
  compressed_data_length = input.ReadAllData((void**)&compressed_data);
  source_manager = (jpeg_source_mgr*)
    (*info.mem->alloc_small) ((j_common_ptr)&info, JPOOL_PERMANENT, sizeof(jpeg_source_mgr));
  source_manager->init_source = JpegMemoryInitSource;
  source_manager->fill_input_buffer = JpegMemoryFillInputBuffer;
  source_manager->skip_input_data = JpegMemorySkipInputData;
  source_manager->resync_to_restart = jpeg_resync_to_restart;
  source_manager->term_source = JpegMemoryTerminateSource;
  source_manager->next_input_byte = compressed_data;
  source_manager->bytes_in_buffer = compressed_data_length;
  info.src = source_manager;

  // Decompress the jpg image
	jpeg_read_header(&info, true);
  jpeg_start_decompress(&info);
	width = info.image_width;
	height = info.image_height;
	bpp = info.num_components*8;
  if (width < 0 || height < 0 || width > kMaxImageWidth || height > kMaxImageHeight ||
      (bpp != 24 && bpp != 32))
    goto error;
	pixels = (unsigned char *)malloc(width * height * bpp / 8);
  for (int y = 0; y < height; y++) {
    unsigned char *row = pixels + y * width * bpp / 8;
    jpeg_read_scanlines(&info, &row, 1);
  }
  jpeg_finish_decompress(&info);
  result = new Image(pixels, width, height, bpp);
error:
  if (pixels != 0)
    free(pixels);
  if (compressed_data != 0)
    free(compressed_data);
  jpeg_destroy_decompress(&info);
  return result;
  return 0;
}

// Tga loading
// ===========

bool Image::IsTga(InputStream input) {
  unsigned char tag[3];
  bool is_tga = (input.LookAheadReadChars(0, 3, tag) == 3 && tag[1] == 0 &&
                 (tag[2] == 2 || tag[2] == 10));
  return is_tga;
}

Image *Image::LoadTga(InputStream input) {
  // Read header data
  int header_to_data_offset = input.ReadChar();
  input.SkipAhead(1);
  bool is_compressed = (input.ReadChar() == 10);
  input.SkipAhead(9);
  int width = input.ReadShort();
  int height = input.ReadShort();
  int bpp = input.ReadChar();
  int bytes_per_row = ((width*(bpp/8)+3)/4)*4;
  input.SkipAhead(header_to_data_offset + 1); 

  // Read an uncompressed image
  unsigned char *pixels = new unsigned char[height*bytes_per_row];
  if (!is_compressed) {
    // Read the data
    if (input.ReadChars(height * width * bpp/8, pixels) != height*width*bpp/8) {
      delete pixels;
      return 0;
    }

    // Convert from BGR to RGB
    for (int i = 0; i < width*height; i++) {
      char temp = pixels[i*(bpp/8) + 0];
      pixels[i*(bpp/8) + 0] = pixels[i*(bpp/8) + 2];
      pixels[i*(bpp/8) + 2] = temp;
    }
  }

  // Read a run-length encoded image
  else {
    int pos = 0, data;
    char new_color[4];
    while (pos < width*height) {
      data = input.ReadChar();

      // Handle non-encoded data
      if (data < 128) {
        for (int i = 0; i <= data; i++) {
          input.ReadChars(bpp/8, new_color);
          if (bpp >= 24) {
            char temp = new_color[0];
            new_color[0] = new_color[2];
            new_color[2] = temp;
          }
          memcpy(&pixels[pos*(bpp/8)], new_color, bpp/8);
          pos++;
        }
      }

      // Handle encoded data
      else {
        input.ReadChars(bpp/8, new_color);
        if (bpp >= 24) {
          char temp = new_color[0];
          new_color[0] = new_color[2];
          new_color[2] = temp;
        }
        for (int i = 128; i <= data; i++) {
          memcpy(&pixels[pos*(bpp/8)], new_color, bpp/8);
          pos++;
        }
      }
    }
  }
  return new Image(pixels, width, height, bpp);
}

// Png loading
// ===========

bool Image::IsPng(InputStream input) {
  unsigned char tag[8];
  bool is_png = (input.LookAheadReadChars(0, 8, tag) == 8 && !png_sig_cmp(tag, 0, 8));
  return is_png;
}

void input_reader(png_structp png_ptr, png_bytep data, png_size_t length) {
  InputStream *istr = (InputStream*)png_get_io_ptr(png_ptr);
  
  int ct = istr->ReadChars(length, data);
  if(ct != length)
      png_error(png_ptr, "unexpected EOF");
}
  

Image *Image::LoadPng(InputStream input) {
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  ASSERT(png_ptr);

  png_infop info_ptr = png_create_info_struct(png_ptr);
  ASSERT(info_ptr);
  
  if(setjmp(png_jmpbuf(png_ptr))) {
    ASSERT(0);
  }
  
  png_set_read_fn(png_ptr, &input, input_reader);
  
  png_read_info(png_ptr, info_ptr);
  
  if(info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);
  if(info_ptr->color_type == PNG_COLOR_TYPE_GRAY && info_ptr->bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png_ptr);
  if(info_ptr->bit_depth == 16)
    png_set_strip_16(png_ptr);
  if(info_ptr->bit_depth < 8)
    png_set_packing(png_ptr);
  if(info_ptr->color_type == PNG_COLOR_TYPE_RGB)
    png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
  if(info_ptr->color_type == PNG_COLOR_TYPE_GRAY || info_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  png_read_update_info(png_ptr, info_ptr);

  ASSERT(info_ptr->bit_depth == 8);
  ASSERT(info_ptr->color_type == PNG_COLOR_TYPE_RGBA || info_ptr->color_type == PNG_COLOR_TYPE_RGB);
  
  unsigned char *pixels = new unsigned char[info_ptr->width * info_ptr->height * 4];
  
  vector<unsigned char *> ul;
  for(int i = 0; i < info_ptr->height; i++)
    ul.push_back(pixels + i * info_ptr->width * 4);
  
  png_read_image(png_ptr, (png_byte**)&ul[0]);
  
  Image *q = new Image(pixels, info_ptr->width, info_ptr->height, 32);

  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  
  return q;
}

