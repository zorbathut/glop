#include "../include/Image.h"
#include "../include/BinaryFileManager.h"
#include "../include/Color.h"
#include "third_party/jpeglib/jpeglib.h"
#include <setjmp.h>  // For LoadJpg error handling

// Constants
const int kMaxImageWidth = 65536;
const int kMaxImageHeight = 65536;

// See Image.h
Image *Image::Load(BinaryFileReader reader, bool force_alpha) {
  if (!reader.IsOpen())
    return 0;
  if (IsBmp(reader))
    return LoadBmp(reader, force_alpha);
  if (IsGif(reader))
    return LoadGif(reader, force_alpha);
  if (IsJpg(reader))
    return LoadJpg(reader, force_alpha);
  return 0;
}

// See Image.h
Image *Image::Load(BinaryFileReader reader, const Color &bg_color, int bg_tolerance) {
  // First load the image normally (but in 32 bit format)
  Image *result = Load(reader, true);
  if (result == 0)
    return 0;

  // Set the alpha channel based on the background
  unsigned char *pixels = result->GetPixels();
  int x, y, w = result->GetWidth(), h = result->GetHeight();
  int target[] = {(int)(bg_color[0]*255), (int)(bg_color[1]*255), (int)(bg_color[2]*255)};
  for (y = 0; y < h; y++)
  for (x = 0; x < w; x++) {
    unsigned char *pixel = pixels + y*result->GetBytesPerRow() + x*4;
    int d = abs(pixel[0] - target[0]) + abs(pixel[1] - target[1]) + abs(pixel[2] - target[2]);
    if (d <= bg_tolerance)
      pixel[3] = 0;
    else
      pixel[3] = 255;
  }
  result->SmoothTransparentColors();
  return result;
}

// Returns a scaled version of this image - the trivial nearest neighbor scaling algorithm is used.
Image *Image::ScaledImage(const Image *image, int new_width, int new_height) {
  int bpp = image->GetBpp();
  unsigned char *pixels = (unsigned char*)malloc(new_width * new_height * bpp);
  for (int y = 0; y < new_height; y++)
  for (int x = 0; x < new_width; x++)
  for (int c = 0; c < bpp; c++) {
    int x2 = x * image->GetWidth() / new_width;
    int y2 = y * image->GetHeight() / new_height;
    pixels[(y*new_width+x)*bpp/8 + c] = image->pixels_[y2*image->GetBytesPerRow() + x2*bpp/8 + c];
  }
  Image *result = new Image(pixels, new_width, new_height, bpp, false);
  free(pixels);
  return result;
}

// Constructor - the image should already be loaded at this point.
// We enlarge it to a power of 2, however, and set the pixels adjacent to boundaries to be equal
// to the boundary color, which helps with OpenGl scaling (see SmoothTransparentColors).
Image::Image(unsigned char *data, int width, int height, int bpp, bool force_alpha)
: pixels_(0),
  width_(width),
  height_(height),
  bpp_(force_alpha? 32 : bpp) {
  internal_width_ = NextPow2(width);
  internal_height_ = NextPow2(height);
  pixels_ = new unsigned char[internal_width_ * internal_height_ * bpp_ / 8];
  for (int y = 0; y < internal_height_; y++)
  for (int x = 0; x < internal_width_; x++)
  for (int c = 0; c < bpp_/8; c++) {
    int tx = (x == width_? width_ - 1 : x == internal_width_ - 1? 0 : x);
    int ty = (y == height_? height_ - 1 : y == internal_height_ - 1? 0 : y);
    if (ty < height && tx < width && c < bpp/8)
      pixels_[y*internal_width_*bpp_/8 + x*bpp_/8 + c] = data[ty*width*bpp/8 + tx*bpp/8 + c];
    else
      pixels_[y*internal_width_*bpp_/8 + x*bpp_/8 + c] = 0;
  }
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
// lead to borders around the image. To prevent this, we use this to set transparent pixel colors
// to be the average of the non-transparent colors around the pixel.
void Image::SmoothTransparentColors() {
  if (bpp_ != 32)
    return;
  int w = internal_width_, h = internal_height_;
  for (int y = 0; y < h; y++)
  for (int x = 0; x < w; x++) {
    unsigned char *pixel = pixels_ + y*GetBytesPerRow() + x*4;
    if (pixel[3] != 0)
      continue;
    int rt = 0, gt = 0, bt = 0, t = 0;
    for (int y2 = -1; y2 <= 1; y2++)
    for (int x2 = -1; x2 <= 1; x2++) {
      unsigned char *pixel2 = pixels_ + ((y+y2+h)%h)*GetBytesPerRow() + ((x+x2+w)%w)*4;
      if (pixel2[3] == 0)
        continue;
      rt += pixel2[0];
      gt += pixel2[1];
      bt += pixel2[2];
      t++;
    }
    if (t == 0) {
      pixel[0] = pixel[1] = pixel[2] = 0;
    } else {
      pixel[0] = rt / t;
      pixel[1] = gt / t;
      pixel[2] = bt / t;
    }
  }
}

// Bmp loading
// ===========

// Returns whether a reader is pointing to a BMP file. The file pointer location is not changed.
bool Image::IsBmp(BinaryFileReader reader) {
  int start_offset = reader.Tell();
  char tag[2];
  bool is_bmp = (reader.ReadChars(2, tag) == 2 && tag[0] == 'B' && tag[1] == 'M');
  reader.Seek(start_offset, BinaryFileReader::Start);
  return is_bmp;
}

// Loads a BMP file. Supports 1, 4, 8, 16, 24, and 32 bit uncompressed pixels.
// Adapted from the SDL image library.
Image *Image::LoadBmp(BinaryFileReader reader, bool force_alpha) {
  const int kRgb = 0;
  const int kRle8 = 1;
  const int kRle4 = 2;
  unsigned int rmask = 0, gmask = 0, bmask = 0, amask = 0;
  unsigned int rshift = 1, gshift = 1, bshift = 1, ashift = 1;

  // Variables - these must all be defined up here so that we can goto error
  Image *result = 0;                        // The return value
  unsigned char *palette = 0, *pixels = 0;  // Image data (palette is used if bpp <= 8)
  unsigned char *buffer = 0;                // A location to load a line of data into
  int start_offset = reader.Tell();         // Where does the file begin?

  // Read the header
  char tag[2];
  if (reader.ReadChars(2, tag) < 2 || tag[0] != 'B' || tag[1] != 'M')
    goto error;
  reader.Seek(8, BinaryFileReader::Current);
  int image_start, header_size;
  if (!reader.ReadInts(1, &image_start) ||
      !reader.ReadInts(1, &header_size))
    goto error;
  int width, height, compression;
  short bpp;
	if (header_size == 12) {
    unsigned short short_width, short_height;
    if (!reader.ReadShorts(1, &short_width) ||
        !reader.ReadShorts(1, &short_height))
      goto error;
    width = short_width;
    height = short_height;
    reader.Seek(2, BinaryFileReader::Current);
    if (!reader.ReadShorts(1, &bpp))
      goto error;
    compression = kRgb;
	} else {
    if (!reader.ReadInts(1, &width) ||
        !reader.ReadInts(1, &height))
      return 0;
    reader.Seek(2, BinaryFileReader::Current);
    if (!reader.ReadShorts(1, &bpp) ||
        !reader.ReadInts(1, &compression))
      return 0;
    reader.Seek(20, BinaryFileReader::Current);
	}
  if (bpp != 1 && bpp != 4 && bpp != 8 && bpp != 15 && bpp != 16 && bpp != 24 && bpp != 32)
    return 0;
  if (width < 0 || width > kMaxImageWidth || height < 0 || height > kMaxImageHeight)
    return 0;
  int new_bpp = (bpp == 32? 32 : 24);

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
        if (!reader.ReadInts(1, &rmask) || !reader.ReadInts(1, &gmask) ||
            !reader.ReadInts(1, &bmask) || !reader.ReadInts(1, &amask))
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
    reader.Seek(start_offset + 14 + header_size, BinaryFileReader::Start);
		for (int i = 0; i < colors_used; i++) {
      if (!reader.ReadChars(1, palette+3*i+2) ||
          !reader.ReadChars(1, palette+3*i+1) ||
          !reader.ReadChars(1, palette+3*i+0))
        goto error;
      if (header_size != 12 && !reader.ReadChars(1, tag)) // Dummy character
        goto error;
    }
	}

	// Read the pixel data, and convert it to RGB (or RGBA).
  // Note that the image is stored upside-down.
  reader.Seek(start_offset + image_start, BinaryFileReader::Start);
  pixels = (unsigned char*)malloc(width * height * new_bpp / 8);
  if (compression == kRle4 || compression == kRle8) {
    int x = 0, y = height - 1;
    unsigned char dx, dy;
    unsigned char ch, data, pixel;
    bool is_done = false;
    while (!is_done) {
      if (!reader.ReadChars(1, &ch))
        goto error;
      if (ch > 0) {
        // Handle runs (with RLE4, it is a run of period 2 instead of period 1)
        if (!reader.ReadChars(1, &data))
          goto error;
        for (int i = 0; i < ch; i++) {
          if (compression == kRle4)
            pixel = (i % 2 == 0? data / 16 : data % 16);
          memcpy(pixels + y*width*3 + (x++)*3, palette + 3*pixel, 3);
        }
		  } else {
        // Handle escape copes
        if (!reader.ReadChars(1, &ch))
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
            if (!reader.ReadChars(1, &dx) || !reader.ReadChars(1, &dy))
              goto error;
            x += dx;
            y -= dy;
            if (x >= width || y < 0)
              goto error;
				    break;
			    default: // Uncompressed data
            if (compression == kRle8) {
              for (int i = 0; i < ch; i++) {
                if (!reader.ReadChars(1, &data))
                  goto error;
                memcpy(pixels + y*width*3 + (x++)*3, palette + data*3, 3);
              }
              if (ch % 2 == 1) {
                if (!reader.ReadChars(1, &data))
                  goto error;
              }
            } else {
              unsigned char data;
              for (int i = 0; i < ch; i++) {
                if (i % 2 == 0) {
                  if (!reader.ReadChars(1, &data))
                    goto error;
                } else {
                  data *= 16;
                }
                unsigned char pixel = data / 16;
                memcpy(pixels + y*width*3 + (x++)*3, palette + pixel, 3);
              }
              if (((ch+1)/2) % 2 == 1) {
                if (!reader.ReadChars(1, &data))
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
        if (reader.ReadChars(line_length, buffer) < line_length)
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
        if (reader.ReadChars(width * bpp / 8, buffer) < width * bpp / 8)
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

  result = new Image(pixels, width, height, new_bpp, force_alpha);
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

// Returns whether a reader is pointing to a GIF file. The file pointer location is not changed.
bool Image::IsGif(BinaryFileReader reader) {
  int start_offset = reader.Tell();
  unsigned char tag[6];
  bool is_gif = (reader.ReadChars(6, tag) == 6 && memcmp(tag, "GIF", 3) == 0 &&
                 (memcmp(tag+3, "87a", 3) == 0 || (memcmp(tag+3, "89a", 3) == 0)));
  reader.Seek(start_offset, BinaryFileReader::Start);
  return is_gif;
}

// Gif helper function - loads a value with the given number of bits from the file.
static int LoadGifBits(BinaryFileReader &reader, unsigned char *buffer, int num_bits,
                       int *buffer_pos, int *buffer_end) {
  if (*buffer_pos + num_bits >= *buffer_end) {
    if (*buffer_end >= 16)
      buffer[0] = buffer[*buffer_end/8 - 2];
    if (*buffer_end >= 8)
      buffer[1] = buffer[*buffer_end/8 - 1];
    unsigned char block_size;
    if (!reader.ReadChars(1, &block_size))
      return -1;
    if (reader.ReadChars(block_size, buffer + 2) < block_size)
      return -1;
    *buffer_pos = 16 - (*buffer_end - *buffer_pos);
    *buffer_end = 8 * (int(block_size) + 2);
  }
  int value = 0;
  for (int i = *buffer_pos, j = 0; j < num_bits; i++, j++)
	  value |= (((buffer[i / 8] & (1 << (i % 8))) > 0) << j);
  *buffer_pos += num_bits;
  return value;
}

// Loads a GIF file.
// Adapted from the SDL image library, which is, in turn, taken from XPaint.
Image *Image::LoadGif(BinaryFileReader reader, bool force_alpha) {
  const int kMaxLwzBits = 12;
  unsigned char *palette = (unsigned char*)malloc(768), *pixels = 0;
  Image *result = 0;

  // Read the header
  unsigned char block_size;
  unsigned char buffer[260];
  int transparent_index = -1;
  unsigned short width, height, bpp;
  if (reader.ReadChars(6, buffer) < 6)
    goto error;
  if (memcmp(buffer, "GIF", 3) != 0)
    goto error;
  reader.Seek(4, BinaryFileReader::Current);
  unsigned char data;
  if (!reader.ReadChars(1, &data))
    goto error;
  bpp = 1 + (data % 8);
  reader.Seek(2, BinaryFileReader::Current);

  // Read the global palette if it exists
  if ((data & 0x80) > 0) {
    if (reader.ReadChars(3*(1<<bpp), palette) < 3*(1<<bpp))
      goto error;
  }

  // Read GIF data
  while (1) {
    if (!reader.ReadChars(1, &data))
      goto error;
    if (data == '!') {
      // Extension
      if (!reader.ReadChars(1, &data))
        goto error;
      if (data == 0xf9) { // Graphic control extension
        if (!reader.ReadChars(1, &block_size))
          goto error;
        if (reader.ReadChars(block_size, buffer) < block_size)
          goto error;
        if (buffer[0] % 2 == 1)
          transparent_index = buffer[3];
      }
      do {
        if (!reader.ReadChars(1, &block_size))
          goto error;
        if (reader.ReadChars(block_size, buffer) < block_size)
          goto error;
      } while (block_size > 0);
    } else if (data == ',') {
      // Image start - read the header and the local palette if it exists
      reader.Seek(4, BinaryFileReader::Current);
      if (!reader.ReadShorts(1, &width) || !reader.ReadShorts(1, &height) ||
          !reader.ReadChars(1, &data))
        goto error;
      if ((data & 0x80) > 0) {
        bpp = 1 + (data % 8);
        if (reader.ReadChars(3*(1<<bpp), palette) < 3*(1<<bpp))
          goto error;
      }
      bool is_interlaced = (data & 0x40) > 0;
      int new_bpp = (transparent_index == -1? 24 : 32);
      unsigned char code_size;
      if (!reader.ReadChars(1, &code_size))
        return 0;
      pixels = (unsigned char*)malloc(width * height * new_bpp / 8);
      
      // Read a byte
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
                        LoadGifBits(reader, buffer, code_size + 1, &buffer_pos, &buffer_end));
            if (code == -1)
              goto error;
            if (code == clear_code) {
              // Initialization
              clear_code = (1 << code_size);
              max_code = clear_code + 2;
              stack_pos = -1;
    	        for (int i = 0; i < (1 << kMaxLwzBits); ++i) {
		            table[0][i] = 0;
                table[1][i] = (i < clear_code? i : 0);
	            }
              do {
                first_code = LoadGifBits(reader, buffer, code_size + 1, &buffer_pos, &buffer_end);
                if (first_code == -1)
                  goto error;
              } while (first_code == (1 << code_size));
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
              if (max_code >= (1 << (code_size+1)) && code_size+1 < kMaxLwzBits)
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
      result = new Image(pixels, width, height, new_bpp, force_alpha);
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

// Returns whether a reader is pointing to a JPG file. The file pointer location is not changed.
bool Image::IsJpg(BinaryFileReader reader) {
  int start_offset = reader.Tell();
  unsigned char tag[3];
  bool is_jpg = (reader.ReadChars(3, tag) == 3 && tag[0] == 0xFF &&
                 tag[1] == 0xD8 && tag[2] == 0xFF);
  reader.Seek(start_offset, BinaryFileReader::Start);
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

// Loads a JPG file using jpeglib.
Image *Image::LoadJpg(BinaryFileReader reader, bool force_alpha) {
  unsigned char *pixels = 0;
  Image *result = 0;
  jpeg_decompress_struct info;

  // Set up the error handler. If jpeglib runs into an error at any future point, it will
  // execute the block after setjmp(), which will otherwise remain unexecuted.
  CustomJpgErrorManager error_manager;
  info.err = jpeg_std_error(&error_manager.standard_manager);
  error_manager.standard_manager.error_exit = OnJpgErrorExit;
  error_manager.standard_manager.output_message = OnJpgErrorMessage;
  if (setjmp(error_manager.jump_location))
    goto error;

  // Load the jpg image
	jpeg_create_decompress(&info);
  jpeg_stdio_src(&info, (FILE*)reader.GetFilePointer());
	jpeg_read_header(&info, true);
  jpeg_start_decompress(&info);
	int width = info.image_width;
	int height = info.image_height;
	int bpp = info.num_components*8;
  if (width < 0 || height < 0 || width > kMaxImageWidth || height > kMaxImageHeight ||
      (bpp != 24 && bpp != 32))
    goto error;
	pixels = (unsigned char *)malloc(width * height * bpp / 8);
  for (int y = 0; y < height; y++) {
    unsigned char *row = pixels + y * width * bpp / 8;
    jpeg_read_scanlines(&info, &row, 1);
  }
  jpeg_finish_decompress(&info);
  if (reader.Tell() > reader.GetLength())
    goto error;
  result = new Image(pixels, width, height, bpp, force_alpha);

error:
  if (pixels != 0)
    free(pixels);
  jpeg_destroy_decompress(&info);
  return result;
}
