#ifndef GLOP_SYSTEM_H__
#define GLOP_SYSTEM_H__

// Includes
#include "Base.h"
#include <vector>

// Class declarations
class Color;
class GlopWindow;
class System;
#ifndef GLOP_LEAN_AND_MEAN
class SoundManager;
#endif // GLOP_LEAN_AND_MEAN

// Globals
System *system();

// System class definition
class System {
 public:
  // Startup. Creates system() and does all setup we want. ShutDown is done automatically. Many
  // Glop functions require this to be called before they are used.
  static void Init();
 
  // Shutdown. Manual shutdown is currently provided because the automatic thingy wasn't working properly.
  static void ShutDown();

  // Internal logic - Think must be called exactly once per frame. It returns the number of
  // milliseconds that have elapsed since the previous call. During the call to Think, all
  // KeyHandlers receive OnKeyEvent messages, all GlopFrames perform all their logic, and all
  // rendering is performed. See GlopFrameBase.h for a more detailed pipeline.
  int Think();

  // Message boxes
  // =============

  // Displays a highly-visible modal (i.e. freezes program execution until the user clicks okay)
  // dialog box.
  void MessageBox(const string &title, const string &message);
  void MessageBoxf(const string &title, const char *message, ...);

  // Time-keeping
  // ============

  // Returns the number of milliseconds that have elapsed since the program began.
  int GetTime();

  // Returns the number of microseconds that have elapsed since the program began. 
  int64 GetTimeMicro();
  
  // Returns the number of times Think has finished executing since the program began.
  int GetFrameCount() const {return frame_count_;}

  // Causes the current thread to give up context and sleep for the given number of milliseconds.
  // During this time, it will cause 0 load on the CPU. If no time is supplied for Sleep, it
  // sleeps for a fraction of the desired frame speed.
  void Sleep(int t);
  void Sleep() {Sleep(0);}

  // Returns the current frame rate for the program. This is a running average from data over a
  // fixed time interval.
  float GetFps() {return fps_;}

  // Windowing
  // =========
  
  // Returns all full-screen resolutions (width, height) that are supported on this computer. The
  // resolutions are listed in increasing lexicographical order. min_width and min_height may be
  // set to restrict the list to only contain resolutions with at least a certain size.
  vector<pair<int, int> > GetFullScreenModes(int min_width = 640, int min_height = 480);

  // Returns the main window for this Glop program. This can also be gotten via the window() global
  // function (see GlopWindow.h).
  GlopWindow *window() {return window_;}
  
  // Sound
  // =====

  #ifndef GLOP_LEAN_AND_MEAN
  // Returns the sound manager for this Glop program. This can also be gotten via the
  // sound_manager() global function (see Sound.h).
  SoundManager *sound_manager() {return sound_manager_;}
  #endif // GLOP_LEAN_AND_MEAN

  // File system
  // ===========

  // Returns all files in the given directory matching one of the given suffixes (or all files if
  // suffixes is empty). Wildcards are not permitted. Hidden files are ignored.
  vector<string> ListFiles(const string &directory,
                           const vector<string> &suffixes = vector<string>(0));
  vector<string> ListFiles(const string &directory, string suffix) {
    return ListFiles(directory, vector<string>(1, suffix));
  }

  // Returns all subdirectories of the given directory. Hidden directories are ignored.
  vector<string> ListSubdirectories(const string &directory);
private:
  System();
  ~System();
  
  // General data
  GlopWindow *window_;
  #ifndef GLOP_LEAN_AND_MEAN
  SoundManager *sound_manager_;
  #endif // GLOP_LEAN_AND_MEAN
  int frame_count_;
  int refresh_rate_query_delay_, refresh_rate_;
  int vsync_time_;             // Time spent waiting for vsync last frame
  int start_time_, old_time_;  // Os::Time as of program start and as of the last call to Think
  int64 start_micro_time_;
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
