// Includes
#include "GlopInternalData.h"
#include "OpenGl.h"
#include "third_party/freetype/ftglyph.h"

// Globals
void *FreeTypeLibrary::library_ = 0;
List<Texture*> GlDataManager::textures_;
List<DisplayList*> GlDataManager::display_lists_;
List<DisplayLists*> GlDataManager::multi_display_lists_;

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
// GlDataManager
// =============

void GlDataManager::GlInitAll() {
  for (List<Texture*>::iterator it = textures_.begin(); it != textures_.end(); ++it)
    (*it)->GlInit();
}

void GlDataManager::GlShutDownAll() {
  for (List<Texture*>::iterator it = textures_.begin(); it != textures_.end(); ++it)
    (*it)->GlShutDown();
  for (List<DisplayList*>::iterator it = display_lists_.begin(); it != display_lists_.end(); ++it)
    (*it)->Clear();
  for (List<DisplayLists*>::iterator it = multi_display_lists_.begin();
       it != multi_display_lists_.end(); ++it)
    (*it)->Clear();
}
