#ifndef GLOP_SYSTEM_H__
#define GLOP_SYSTEM_H__

// Includes
#include "Base.h"
#include "BinaryFileManager.h"
#include <vector>

// Class declarations
class Color;
class GlopWindow;
class Image;
class System;

// Globals
extern System *gSystem;

// System class definition
class System {
 public:
  // Startup. Creates gSystem and does all setup we want. ShutDown is done automatically.
  static void Init();

  // Internal logic - Think must be called exactly once per frame. It returns the number of
  // milliseconds that have elapsed since the previous call. During the call to Think, all
  // KeyHandlers receive OnKeyEvent messages, all GlopFrames perform all their logic, and all
  // rendering is performed. See GlopFrameBase.h for a more detailed pipeline.
  int Think();

  // Time-keeping
  // ============

  // Returns the number of milliseconds that have elapsed since the program began.
  int GetTime();

  // Returns the number of times Think has finished executing since the program began.
  int GetFrameCount() const {return frame_count_;}

  // Causes the current thread to give up context and sleep for the given number of milliseconds.
  // During this time, it will cause 0 load on the CPU. If no time is supplied for Sleep, it
  // sleeps for a fraction of the desired frame speed.
  void Sleep(int t);
  void Sleep() {
    Sleep(max_fps_ == 0? 0 : (250 / max_fps_));
  }

  // Returns the current frame rate for the program. This is a running average from data over a
  // fixed time interval.
  float GetFps() {return fps_;}

  // Gets/sets the maximum frame rate for the program. If this is 0, then no artificial frame rate
  // will be set. Otherwise System::Think will automatically sleep to ensure the frame rate does
  // not exceed this value.
  int GetMaxFps() {return max_fps_;}
  void SetMaxFps(int fps) {max_fps_ = fps;}

  // Windowing
  // =========
  
  // Returns all full-screen resolutions (width, height) that are supported on this computer. The
  // resolutions are listed in increasing lexicographical order. min_width and min_height may be
  // set to restrict the list to only contain resolutions with at least a certain size.
  vector<pair<int, int> > GetFullScreenModes(int min_width = 640, int min_height = 480);

  // Returns the main window for this Glop program. This can also be gotten via the gWindow global
  // variable (see GlopWindow.h).
  GlopWindow *window() {return window_;}
  
 private:
  System();
  static void ShutDown() {delete gSystem;}
  ~System();
  
  // General data
  GlopWindow *window_;
  int max_fps_;
  int frame_count_;
  int start_time_, old_time_;  // Os::Time as of program start and as of the last call to Think
  void *free_type_library_;    // Internal handle to the FreeType library, used for text

  // FPS data
  static const int kFpsHistorySize = 20;
  float fps_;                              // Our frame rate as of the last calculation
  bool fps_history_filled_;                // Have we made at least kFpsHistorySize measurements?
  int fps_array_index_;                    // What's the next entry in the history to fill?
  int fps_frame_history_[kFpsHistorySize]; // Frame and time measurements. Measurements are made at
  int fps_time_history_[kFpsHistorySize];  //  time intervals, not after a fixed number of frames,
                                           //  which is why fps_frame_history is needed.
  
  DISALLOW_EVIL_CONSTRUCTORS(System);
};

#endif // SYSTEM_H__
