// The main window in which all Glop actions take place. The GlopWindow class is directly
// responsible for talking with the operating system to configure the window (icon, title, size,
// etc.) Input handling and rendering are delegated to Input and GlopFrame's respectively.
//
// Glop supports only one window, and the object is owned by System. To actually have a window
// appear on screen, Window::Create needs to be called.

#ifndef GLOP_GLOP_WINDOW_H__
#define GLOP_GLOP_WINDOW_H__

// Includes
#include "Base.h"
#include "GlopFrameBase.h"
#include "LightSet.h"
using namespace std;

// Class declarations
class GlopWindow;
class Image;
class Input;
struct KeyEvent;
struct OsWindowData;

// GlopWindowSettings class definition
struct GlopWindowSettings {
  GlopWindowSettings()
  : stencil_bits(0),
    is_resizable(true),
    min_width(128),
    min_height(128),
    min_aspect_ratio((4.0f / 3) * 0.3f),
    min_inverse_aspect_ratio((3.0f / 4) * 0.3f) {}
  int stencil_bits;
  bool is_resizable;              // Only affects windowed mode
  int min_width, min_height;      // Minimum window sizes - used to prevent user screwing things
                                  //  up. Particularly useful since some (all?) Win32 computers
                                  //  seem to have bugs with height < 15.
  float min_aspect_ratio,         // Similar to min_width and min_height: Lower bounds on
        min_inverse_aspect_ratio; //  width/height and 1/(width/height).
};

// Globals
extern GlopWindow *gWindow;

// GlopWindow class definition
class GlopWindow {
 public:
  // Input accessor. The non-class method input() (defined in Input.h) may also be used.
  const Input *input() const {return input_;}
  Input *input() {return input_;}

  // Window mutators
  // ===============

  // Creates this window with the given settings. If the window is already created, it will be
  // destroyed and recreated with the new settings. Note that no data is lost when this happens.
  // On failure, the window will automatically be returned to its original state. 
  // Returns: whether the window creation was successful.
  bool Create(int width, int height, bool full_screen,
              const GlopWindowSettings &settings = GlopWindowSettings());

  // Destroys the window. While destroyed, the window will not generate input, will not render, and
  // will not allow frames to perform logic. However, it can be recreated at any time.
  void Destroy();

  // Changes the text of the window title. This will work either before or after the window is
  // created.
  void SetTitle(const string &title);

  // Changes the window icon to be the given image (possibly 0 for a default icon). This will work
  // either before or after the window is created. The icon image is owned by the caller, but it
  // should not be deleted while it remains set as the window icon.
  void SetIcon(const Image *icon);

  // Window accessors
  // ================
  //
  // With the exception of IsCreated, all values are defined only if the window is created

  // Returns whether the window is currently in existence.
  bool IsCreated() const {return is_created_;}

  // Returns the size of the window.
  int GetWidth() const {return width_;}
  int GetHeight() const {return height_;}

  // Returns whether the window is currently running in full-screen mode (i.e. it has changed
  // the desktop resolution and hidden all other programs).
  bool IsFullScreen() const {return is_full_screen_;}

  // Returns the user-requested additional settings for the window.
  const GlopWindowSettings &GetSettings() const {return settings_;}

  // Returns whether the window is currently the target for user input.
  bool IsInFocus() const {return is_in_focus_;}
  
  // Returns whether the window is currently minimized (invisible except for being on the taskbar
  // or operating system equivalent).
  bool IsMinimized() const {return is_minimized_;}

  // Returns the current title of the window.
  const string &GetTitle() const {return title_;}

  // Returns the image icon that the user requested for this window. If the user has not requested
  // any icon, 0 is returned.
  const Image *GetIcon() {return icon_;}
  
  // Frame accessors
  // ===============
  //
  // See TableauFrame in GlopFrameBase.h
  const GlopFrame *GetFrame(LightSetId id) const;
  GlopFrame *GetFrame(LightSetId id);
  LightSetId GetFirstFrameId() const;
  LightSetId GetNextFrameId(LightSetId id) const;
  float GetFrameRelX(LightSetId id) const;
  float GetFrameRelY(LightSetId id) const;
  int GetFrameDepth(LightSetId id) const;
  float GetFrameHorzJustify(LightSetId id) const;
  float GetFrameVertJustify(LightSetId id) const;
  
  // Frame mutators
  // ==============
  //
  // See TableauFrame in GlopFrameBase.h
  LightSetId AddFrame(GlopFrame *frame, float rel_x, float rel_y,
                      float horz_justify, float vert_justify, int depth = 0);
  LightSetId AddFrame(GlopFrame *frame, int depth = 0) {
    return AddFrame(frame, 0.5f, 0.5f, kJustifyCenter, kJustifyCenter, depth);
  }
  void MoveFrame(LightSetId id, int depth);
  void MoveFrame(LightSetId id, float rel_x, float rel_y);
  void MoveFrame(LightSetId id, float rel_x, float rel_y, int depth);
  void SetFrameJustify(LightSetId id, float horz_justify, float vert_justify);
  GlopFrame *RemoveFrameNoDelete(LightSetId id);
  void RemoveFrame(LightSetId id);
  void ClearFrames();
  
  // See GlopFrameBase.h
  void PushFocus();
  void PopFocus();

 private:
  // Interface to System
  friend class System;
  GlopWindow();
  ~GlopWindow();
  void Think(int dt);
 
  // Interface to GlopFrame
  friend class GlopFrame;
  void UnregisterAllPings(GlopFrame *frame);
  void RegisterPing(GlopFrame::Ping *ping) {ping_list_.InsertItem(ping);}

  // Interface to Input
  friend class Input;
  void OnKeyEvent(const KeyEvent &event, int dt);

  // Resizing
  void ChooseValidSize(int width, int height, int *new_width, int *new_height);

  // Focus utilities - see GlopFrameBase.h
  friend class FocusFrame;
  int RegisterFocusFrame(FocusFrame *frame);
  void UnregisterFocusFrame(int layer, FocusFrame *frame);
  void DemandFocus(int layer, FocusFrame *frame, bool update_is_gaining_focus);
  
  // Configuration data
  OsWindowData *os_data_;        // OS handle on this window - needed for all OS calls
  bool is_created_;
  int width_, height_;
  bool is_full_screen_;
  GlopWindowSettings settings_;
  string title_;
  const Image *icon_;

  // Additional tracked data
  bool is_in_focus_, is_minimized_; // See window accessors above
  bool recreated_this_frame_;       // Was ::Create called this frame? If so, we reset the input.
  int windowed_x_, windowed_y_;     // Window position as of when we were last in windowed mode -
                                    //  used to reset position after switching out of fullscreen.
                                    //  Values of -1 indicate no value has ever been recorded.

  // Content data
  LightSet<GlopFrame::Ping*> ping_list_;
  vector<FocusFrame*> focus_stack_;
  TableauFrame *frame_;
  Input *input_;
  DISALLOW_EVIL_CONSTRUCTORS(GlopWindow);
};

#endif // GLOP_GLOP_WINDOW_H__
