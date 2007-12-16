#ifndef GLOP_INTERNAL_DATA_H__
#define GLOP_INTERNAL_DATA_H__

// Includes
#include "../include/LightSet.h"

// Class declarations
class DisplayList;
class DisplayLists;
class Texture;

// FreeTypeLibrary class definition. This initializes and returns a FreeType interface on demand.
class FreeTypeLibrary {
 public:
  static void *Get();
  static void ShutDown();
 private:
  static void *library_;
};

// GlDataManager class definition. Stores all OpenGl data that is tied to a specific window. When
// the data needs to be reset because of a window being created or deleted, it is done through
// GlDataManager.
class GlDataManager {
 public:
  static void GlInitAll();
  static void GlShutDownAll();

  static LightSetId RegisterTexture(Texture *texture) {
    return textures_.InsertItem(texture);
  }
  static void UnregisterTexture(LightSetId id) {
    textures_.RemoveItem(id);
  }
  static LightSetId RegisterDisplayList(DisplayList *dlist) {
    return display_lists_.InsertItem(dlist);
  }
  static void UnregisterDisplayList(LightSetId id) {
    display_lists_.RemoveItem(id);
  }
  static LightSetId RegisterDisplayLists(DisplayLists *dlists) {
    return multi_display_lists_.InsertItem(dlists);
  }
  static void UnregisterDisplayLists(LightSetId id) {
    multi_display_lists_.RemoveItem(id);
  }
 private:
  static LightSet<Texture*> textures_;
  static LightSet<DisplayList*> display_lists_;
  static LightSet<DisplayLists*> multi_display_lists_;
};

#endif // GLOP_INTERNAL_DATA_H__
