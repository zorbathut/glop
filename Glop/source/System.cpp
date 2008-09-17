// Includes
#include "System.h"
#include "GlopWindow.h"
#include "Image.h"
#include "Input.h"
#include "OpenGl.h"
#include "GlopInternalData.h"
#include "Os.h"
#include "third_party/freetype/ftglyph.h"
#include <algorithm>

// Globals
static System *gSystem = 0;
System *system() {return gSystem;}

// Glop initialization
void System::Init() {
  Os::Init(); // Can we not init the system before calling Os::Init().  Otherwise can't log in Init.
  gSystem = new System();
  atexit(System::ShutDown);
  Input::InitDerivedKeys();
  InitDefaultFrameStyle(0);
}

// Internal logic - see System.h.
int System::Think() {
  const int kFpsRecordingDelay = 50; // Number of milliseconds between fps updates

  // Update the refresh rate. We do this at regular intervals in case the display setting changes,
  // either by a window switching into or out of full-screen mode or by the user changing the
  // computer system configuration.
  if (refresh_rate_query_delay_ <= 0 && window_->IsVSynced()) {
    refresh_rate_ = Os::GetRefreshRate();
    refresh_rate_query_delay_ = 1000;
  }

  // Calculate the current time, and sleep until we have spent an appropriate amount of time
  // since the last call to Think.
  int ticks = Os::GetTime();
  if (window_->IsVSynced()) {
    int ticks_target = old_time_ + (refresh_rate_ == 0? 0 : 1000 / refresh_rate_) + vsync_time_;
    while (ticks < ticks_target) {
      Os::Sleep(ticks_target - ticks);
      ticks = Os::GetTime();
    }
  }
  int dt = ticks - old_time_;
  refresh_rate_query_delay_ -= dt;
  old_time_ = ticks;
  
  // Calculate the current frame rate
  int last_fps_index = (fps_array_index_ + kFpsHistorySize - 1) % kFpsHistorySize;
  if ((fps_array_index_ == 0 && !fps_history_filled_) ||
      ticks >= fps_time_history_[last_fps_index] + kFpsRecordingDelay) {
    fps_frame_history_[fps_array_index_] = frame_count_;
    fps_time_history_[fps_array_index_] = ticks;
    int oldest_fps_index = (fps_history_filled_? (fps_array_index_ + 1) % kFpsHistorySize: 0);
    if (fps_array_index_ != oldest_fps_index) {
      fps_ =
        (fps_frame_history_[fps_array_index_] - fps_frame_history_[oldest_fps_index]) * 1000.0f /
        (fps_time_history_[fps_array_index_] - fps_time_history_[oldest_fps_index]);
    }
    fps_array_index_ = (fps_array_index_ + 1) % kFpsHistorySize;
    fps_history_filled_ |= (fps_array_index_ == 0);
  }

  // Perform all Os and window logic
  Os::Think();
  vsync_time_ = window_->Think(dt);

  // Update our frame and time counts
  frame_count_++;
  return dt;
}

// Time-keeping
// ============

int System::GetTime() {
  return Os::GetTime() - start_time_;
}

void System::Sleep(int t) {
  Os::Sleep(t);
}

// Windowing
// =========

vector<pair<int, int> > System::GetFullScreenModes(int min_width, int min_height) {
  vector<pair<int, int> > all_modes = Os::GetFullScreenModes(), good_modes;
  for (int i = 0; i < (int)all_modes.size(); i++)
    if (all_modes[i].first >= min_width && all_modes[i].second >= min_height)
      good_modes.push_back(all_modes[i]);
  return good_modes;
}

// Setup
// =====

System::System()
: window_(new GlopWindow()),
  frame_count_(0),
  refresh_rate_query_delay_(0),
  refresh_rate_(0),
  vsync_time_(0),
  start_time_(Os::GetTime()),
  free_type_library_(0),      // The FreeType library is only initialized when needed
  fps_(0),
  fps_history_filled_(false),
  fps_array_index_(0) {
  old_time_ = start_time_;
}

System::~System() {
  delete window_;
  if (free_type_library_ != 0)
    FT_Done_FreeType((FT_Library)free_type_library_);
  FreeTypeLibrary::ShutDown();
  ClearFrameStyle();
  Os::ShutDown();
}
