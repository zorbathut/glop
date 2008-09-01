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
#include "List.h"
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

// Returns the Glop window. Currently there can only be one window.
GlopWindow *window();

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

  // Sets whether rendering to this window should wait for video refresh before rendering. This
  // caps the max frame rate at the video refresh rate, but it should eliminate on-screen tearing.
  // This is off by default.
  void SetVSync(bool is_enabled) {
    if (is_enabled != is_vsync_requested_) {
      is_vsync_requested_ = is_enabled;
      is_vsync_setting_current_ = false;
    }
  }

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

  // Returns whether vsync is enabled. See SetVSync.
  bool IsVSynced() const {return is_vsync_requested_;}

  // Returns the current title of the window.
  const string &GetTitle() const {return title_;}

  // Returns the image icon that the user requested for this window. If the user has not requested
  // any icon, 0 is returned.
  const Image *GetIcon() {return icon_;}
  
  // Frame accessors
  // ===============
  //
  // See TableauFrame in GlopFrameBase.h
  string GetFrameContextString() const;
  const GlopFrame *GetFrame(ListId id) const;
  GlopFrame *GetFrame(ListId id);
  List<GlopFrame*>::const_iterator frame_begin() const;
  List<GlopFrame*>::const_iterator frame_end() const;
  float GetFrameRelX(ListId id) const;
  float GetFrameRelY(ListId id) const;
  int GetFrameDepth(ListId id) const;
  float GetFrameHorzJustify(ListId id) const;
  float GetFrameVertJustify(ListId id) const;
    
  // Frame mutators
  // ==============
  //
  // See TableauFrame in GlopFrameBase.h
  ListId AddFrame(GlopFrame *frame, float rel_x, float rel_y,
                      float horz_justify, float vert_justify, int depth = 0);
  ListId AddFrame(GlopFrame *frame, int depth = 0) {
    return AddFrame(frame, 0.5f, 0.5f, kJustifyCenter, kJustifyCenter, depth);
  }
  void MoveFrame(ListId id, int depth);
  void MoveFrame(ListId id, float rel_x, float rel_y);
  void MoveFrame(ListId id, float rel_x, float rel_y, int depth);
  void SetFrameJustify(ListId id, float horz_justify, float vert_justify);
  GlopFrame *RemoveFrameNoDelete(ListId id);
  void RemoveFrame(ListId id);
  void ClearFrames();

  // Changes the tab order of a FocusFrame. By default, the tab order is given by the order in which
  // FocusFrames are added to the GlopWindow. This is deterministic but potentially hard to control
  // when a GlopFrame and all its descendants are added together. These functions can be used to
  // correct any undesirable orderings caused from this.
  void SetFocusPredecessor(FocusFrame *base, FocusFrame *successor);
  void SetFocusSuccessor(FocusFrame *base, FocusFrame *successor);

  // See GlopFrameBase.h
  const FocusFrame *GetFocusFrame() const {return focus_stack_[focus_stack_.size()-1];}
  FocusFrame *GetFocusFrame() {return focus_stack_[focus_stack_.size()-1];}
  void PushFocus();
  void PopFocus();

 private:
  // Interface to System
  friend class System;
  GlopWindow();
  ~GlopWindow();
  int Think(int dt);
 
  // Interface to GlopFrame
  friend class GlopFrame;
  static void UnregisterAllPings(GlopFrame *frame);
  static void RegisterPing(GlopFrame::Ping *ping);
  static void PropogatePing(GlopFrame::Ping *ping);

  // Interface to Input
  friend class Input;
  void OnKeyEvents(const vector<KeyEvent> &events, int dt);

  // Resizing
  void ChooseValidSize(int width, int height, int *new_width, int *new_height);

  // Focus utilities to be used by FocusFrame - see GlopFrameBase.h
  friend class FocusFrame;
  int RegisterFocusFrame(FocusFrame *frame);
  void UnregisterFocusFrame(FocusFrame *frame);
  void DemandFocus(FocusFrame *frame, bool ping);

  // Focus internal utilities
  void UpdateFramesInFocus();
  void UpdateFramesInFocus(FocusFrame *old_frame, FocusFrame *frame, bool ping);
  FocusFrame *ChooseFocus(FocusFrame *old_focus, const vector<FocusFrame*> &options);
  FocusFrame *GetNextPossibleFocusFrame(FocusFrame *frame);
  FocusFrame *GetPrevPossibleFocusFrame(FocusFrame *frame);
  bool SendKeyEventsToFrame(FocusFrame *frame, const vector<KeyEvent> &events, int dt,
                            bool gained_focus);
  
  // Configuration data
  OsWindowData *os_data_;           // OS handle on this window - needed for all OS calls
  bool is_created_;
  int width_, height_;
  bool is_full_screen_;
  GlopWindowSettings settings_;
  string title_;
  const Image *icon_;
  bool is_vsync_requested_, is_vsync_setting_current_;

  // Additional tracked data
  bool is_in_focus_, is_minimized_; // See window accessors above
  bool recreated_this_frame_;       // Was ::Create called this frame? If so, we reset the input.
  int windowed_x_, windowed_y_;     // Window position as of when we were last in windowed mode -
                                    //  used to reset position after switching out of fullscreen.
                                    //  Values of -1 indicate no value has ever been recorded.

  // Content data
  bool is_resolving_ping_;
  static List<GlopFrame::Ping*> ping_list_;
  vector<FocusFrame*> focus_stack_;
  TableauFrame *frame_;
  Input *input_;
  DISALLOW_EVIL_CONSTRUCTORS(GlopWindow);
};

#endif // GLOP_GLOP_WINDOW_H__
