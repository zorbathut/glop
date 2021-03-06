// Includes
#include "GlopInternalData.h"
#include "OpenGl.h"
#include "freetype/ftglyph.h"

// Globals
#ifndef GLOP_LEAN_AND_MEAN
void *FreeTypeLibrary::library_ = 0;
#endif // GLOP_LEAN_AND_MEAN
List<Texture*> GlDataManager::textures_;
List<DisplayList*> GlDataManager::display_lists_;
List<DisplayLists*> GlDataManager::multi_display_lists_;

#ifndef GLOP_LEAN_AND_MEAN

// FreeTypeLibrary
// ===============

void *FreeTypeLibrary::Get() {
  if (library_ == 0) {
    if (FT_Init_FreeType((FT_Library*)&library_))
      library_ = 0;
  }
  return library_;
}

void FreeTypeLibrary::ShutDown() {
  if (library_ != 0)
    FT_Done_FreeType((FT_Library)library_);
}

#endif // GLOP_LEAN_AND_MEAN

// GlDataManager
// =============

void GlDataManager::GlInitAll() {
  for (List<Texture*>::iterator it = textures_.begin(); it != textures_.end(); ++it)
    (*it)->GlInit();
}

void GlDataManager::GlShutDownAll() {
  for (List<Texture*>::iterator it = textures_.begin(); it != textures_.end(); ++it)
    (*it)->GlShutDown();
  #ifndef IPHONE
  for (List<DisplayList*>::iterator it = display_lists_.begin(); it != display_lists_.end(); ++it)
    (*it)->Clear();
  for (List<DisplayLists*>::iterator it = multi_display_lists_.begin();
       it != multi_display_lists_.end(); ++it)
    (*it)->Clear();
  #endif
}
