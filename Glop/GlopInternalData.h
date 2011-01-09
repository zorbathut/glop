#ifndef GLOP_GLOP_INTERNAL_DATA_H__
#define GLOP_GLOP_INTERNAL_DATA_H__

// Includes
#include "List.h"

// Class declarations
class DisplayList;
class DisplayLists;
class Texture;

#ifndef GLOP_LEAN_AND_MEAN

// FreeTypeLibrary class definition. This initializes and returns a FreeType interface on demand.
class FreeTypeLibrary {
 public:
  static void *Get();
  static void ShutDown();
 private:
  static void *library_;
};

#endif // GLOP_LEAN_AND_MEAN

// GlDataManager class definition. Stores all OpenGl data that is tied to a specific window. When
// the data needs to be reset because of a window being created or deleted, it is done through
// GlDataManager.
class GlDataManager {
 public:
  static void GlInitAll();
  static void GlShutDownAll();

  static ListId RegisterTexture(Texture *texture) {
    return textures_.push_back(texture);
  }
  static void UnregisterTexture(ListId id) {
    textures_.erase(id);
  }
  static ListId RegisterDisplayList(DisplayList *dlist) {
    return display_lists_.push_back(dlist);
  }
  static void UnregisterDisplayList(ListId id) {
    display_lists_.erase(id);
  }
  static ListId RegisterDisplayLists(DisplayLists *dlists) {
    return multi_display_lists_.push_back(dlists);
  }
  static void UnregisterDisplayLists(ListId id) {
    multi_display_lists_.erase(id);
  }
 private:
  static List<Texture*> textures_;
  static List<DisplayList*> display_lists_;
  static List<DisplayLists*> multi_display_lists_;
};

#endif // GLOP_GLOP_INTERNAL_DATA_H__
