// Includes
#include "GlopInternalData.h"
#include "../include/OpenGl.h"
#include "third_party/freetype/ftglyph.h"

// Globals
void *FreeTypeLibrary::library_ = 0;
LightSet<Texture*> GlDataManager::textures_;
LightSet<DisplayList*> GlDataManager::display_lists_;
LightSet<DisplayLists*> GlDataManager::multi_display_lists_;

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
  for (LightSetId id = textures_.GetFirstId(); id != 0; id = textures_.GetNextId(id))
    textures_[id]->GlInit();
}

void GlDataManager::GlShutDownAll() {
  for (LightSetId id = textures_.GetFirstId(); id != 0; id = textures_.GetNextId(id))
    textures_[id]->GlShutDown();
  for (LightSetId id = display_lists_.GetFirstId(); id != 0; id = display_lists_.GetNextId(id))
    display_lists_[id]->Clear();
  for (LightSetId id = multi_display_lists_.GetFirstId(); id != 0;
        id = multi_display_lists_.GetNextId(id))
    multi_display_lists_[id]->Clear();
}