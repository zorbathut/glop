// Includes
#include "../include/Input.h"
#include "../include/GlopWindow.h"
#include "../include/System.h"
#include "../include/Thread.h"
#include "Os.h"

// Constants
const float kBaseMouseSensitivity = 3.0f;
const float kJoystickAxisThreshold = 0.2f;
const int kDoublePressThreshold = 200;
const int kJoystickRefreshDelay = 250;
const char *const kKeyNames[] = {
  "None", "Any",
  0, 0, 0, 0, 0,                                                            // 0 
  0, 0, 0, "Backspace", "Tab",
  0, 0, 0, "Enter", 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, "Escape", 0, 0,
  0, 0, "Space bar", 0, 0,
  0, 0, 0, 0, "'",
  0, 0, 0, 0, ",",
  "-", ".", "/", "0", "1",
  "2", "3", "4", "5", "6",                                                  // 50
  "7", "8", "9", 0, ";",
  0, "=", 0, 0, 0,
  "A", "B", "C", "D", "E",
  "F", "G", "H", "I", "J",
  "K", "L", "M", "N", "O",
  "P", "Q", "R", "S", "T",
  "U", "V", "W", "X", "Y",
  "Z", "[", "\\", "]", 0,
  0, "`", 0, 0, 0,
  0, 0, 0, 0, 0,                                                            // 100
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, "F1",
  "F2", "F3", "F4", "F5", "F6",
  "F7", "F8", "F9", "F10", "F11",
  "F12", 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  "Caps lock", "Num lock", "Scroll lock", "Print screen", "Pause",          // 150
  "Left shift", "Right shift", "Left control", "Right control", "Left alt",
  "Right alt", 0, 0, 0, 0,
  0, "Right", "Left", "Up", "Down",
  "Key pad /", "Key pad *", "Key pad -", "Key pad +", "Key pad enter",
  "Key pad .", "Key pad =", "Key pad 0", "Key pad 1", "Key pad 2",
  "Key pad 3", "Key pad 4", "Key pad 5", "Key pad 6", "Key pad 7",
  "Key pad 8", "Key pad 9", 0, 0, 0,
  "Delete", "Home", "Insert", "End", "Page up",
  "Page down", 0, 0, 0, 0,
  0, 0, 0, 0, 0,                                                            // 200
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,                                                            // 250
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, "Mouse up", "Mouse right", "Mouse down", "Mouse left",
  "Mouse wheel up", "Mouse wheel down", "Left mouse button",
  "Middle mouse button", "Right mouse button"};
const unsigned char kAsciiValues[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 8, 9, 0, 0, 0, 13, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 27, 0, 0, 
  0, 0, 32, 0, 0, 0, 0, 0, 0, '\'', 0, 0, 0, 0, ',', 
  '-', '.', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0, ';',
  0, '=', 0, 0, 0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
  'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
  'Z', '[', '\\', ']', 0, 0, '`', 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, '/', '*', '-', '+', 13, '.', 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const unsigned char kShiftedAsciiValues[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 8, 9, 0, 0, 0, 13, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 27, 0, 0, 
  0, 0, 32, 0, 0, 0, 0, 0, 0, '"', 0, 0, 0, 0, '<', 
  '_', '>', '?', ')', '!', '@', '#', '$', '%', '^', '&', '*', '(', 0, ':',
  0, '+', 0, 0, 0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y',
  'z', '{', '|', '}', 0, 0, '~', 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, '/', '*', '-', '+', 13, '.', 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Static accessor convenience method
Input *input() {
  return gSystem->window()->input();
}

// InputPollingThread
// ==================

// This is a thread devoted to calling Os::PollInput on regular intervals, regardless of how
// fast our frame rate is. While keyboard events should never be lost by the operating system,
// there is no such guarantee on, say, joystick events. Doing regular, fast polling helps prevent
// that from happening.
class InputPollingThread: public Thread {
 public:
  // Constructor - the polling thread will always be tied to the given window.
  InputPollingThread(OsWindowData **os_window_data): os_window_data_(os_window_data) {}

  // Returns all data that has been polled since the last call to GetData. It is guaranteed that
  // Poll will be called at least once.
  vector<vector<Os::KeyEvent> > GetData(vector<int> *time, vector<bool> *is_num_lock_set,
                                        vector<bool> *is_caps_lock_set,
                                        vector<int> *cursor_x, vector<int> *cursor_y,
                                        vector<int> *mouse_dx, vector<int> *mouse_dy) {
    // Ensure the accessing is exclusive with updates, and do at least one poll
    mutex_.Acquire();
    if (key_events_.size() == 0)
      DoPoll();   

    // Get the results
    *time = time_;
    *is_num_lock_set = is_num_lock_set_;
    *is_caps_lock_set = is_caps_lock_set_;
    *cursor_x = cursor_x_;
    *cursor_y = cursor_y_;
    *mouse_dx = mouse_dx_;
    *mouse_dy = mouse_dy_;
    vector<vector<Os::KeyEvent> > result = key_events_;

    // Clear internal aggregated values and return
    key_events_.clear();
    time_.clear();
    is_num_lock_set_.clear();
    is_caps_lock_set_.clear();
    cursor_x_.clear();
    cursor_y_.clear();
    mouse_dx_.clear();
    mouse_dy_.clear();
    mutex_.Release();
    return result;
  }

  // Calls Os::RefreshJoysticks, but blocks while other input related activities are going on.
  void RefreshJoysticks() {
    mutex_.Acquire();
    Os::RefreshJoysticks(*os_window_data_);
    mutex_.Release();
  }

 protected:
  // Attempts to poll
  void Run() {
    while (!IsStopRequested()) {
      mutex_.Acquire();
      DoPoll();
      mutex_.Release();
      gSystem->Sleep(10);
    }
  }

 private:
  // Calls Os::PollInput and aggregates the results.
  void DoPoll() {
    bool temp_num_lock_set, temp_caps_lock_set;
    int temp_cursor_x, temp_cursor_y;
    int temp_mouse_dx, temp_mouse_dy;
    vector<Os::KeyEvent> temp_keys = Os::PollInput(*os_window_data_, &temp_num_lock_set,
                                                   &temp_caps_lock_set, &temp_cursor_x,
                                                   &temp_cursor_y, &temp_mouse_dx, &temp_mouse_dy);
    key_events_.push_back(temp_keys);
    time_.push_back(gSystem->GetTime());
    is_num_lock_set_.push_back(temp_num_lock_set);
    is_caps_lock_set_.push_back(temp_caps_lock_set);
    cursor_x_.push_back(temp_cursor_x);
    cursor_y_.push_back(temp_cursor_y);
    mouse_dx_.push_back(temp_mouse_dx);
    mouse_dy_.push_back(temp_mouse_dy);
  }

  OsWindowData **os_window_data_;
  Mutex mutex_;
  vector<vector<Os::KeyEvent> > key_events_;
  vector<int> time_;
  vector<bool> is_num_lock_set_, is_caps_lock_set_;
  vector<int> cursor_x_, cursor_y_;
  vector<int> mouse_dx_, mouse_dy_;
  DISALLOW_EVIL_CONSTRUCTORS(InputPollingThread);
};

// GlopKey
// =======

const string GlopKey::GetName() const {
  if (joystick == -1) {
    return kKeyNames[index + 2];
  } else {
    string result = Format("Joystick #%d: ", joystick + 1);
    if (index == kJoystickAxisUp)
      result += "up";
    else if (index == kJoystickAxisRight)
      result += "right";
    else if (index == kJoystickAxisDown)
      result += "down";
    else if (index == kJoystickAxisLeft)
      result += "left";
    else if (IsJoystickAxisPos())
      result += Format("axis #%d +", 1 + GetJoystickAxisNumber());
    else if (IsJoystickAxisNeg())
      result += Format("axis #%d -", 1 + GetJoystickAxisNumber());
    else if (IsJoystickHatUp())
      result += Format("hat #%d up", 1 + GetJoystickHatNumber());
    else if (IsJoystickHatRight())
      result += Format("hat #%d right", 1 + GetJoystickHatNumber());
    else if (IsJoystickHatDown())
      result += Format("hat #%d down", 1 + GetJoystickHatNumber());
    else if (IsJoystickHatLeft())
      result += Format("hat #%d left", 1 + GetJoystickHatNumber());
    else if (IsJoystickButton())
      result += Format("button #%d", 1 + GetJoystickButtonNumber());
    else
      ASSERT(false);
    return result;
  }
}

bool GlopKey::IsTrackable() const {
  return *this != kKeyPause && *this != kNoKey && *this != kAnyKey;
}

bool GlopKey::IsKeyboardKey() const {
  return *this != kNoKey && *this != kAnyKey && joystick == -1 && index < kFirstMouseKeyIndex;
}

bool GlopKey::IsMouseKey() const {
  return *this != kNoKey && *this != kAnyKey && joystick == -1 && index >= kFirstMouseKeyIndex;
}

bool GlopKey::IsJoystickKey() const {
  return joystick >= 0;
}

bool GlopKey::IsMouseMotion() const {
  return *this == kMouseUp || *this == kMouseRight || *this == kMouseDown || *this == kMouseLeft ||
         *this == kMouseWheelUp || *this == kMouseWheelDown;
}

bool GlopKey::IsJoystickAxis() const {
  return IsJoystickKey() && index >= kJoystickAxisStart && index < kJoystickAxisEnd;
}

bool GlopKey::IsJoystickAxisPos() const {
  return IsJoystickAxis() && (index - kJoystickAxisPos) % 2 == 0;
}

bool GlopKey::IsJoystickAxisNeg() const {
  return IsJoystickAxis() && (index - kJoystickAxisNeg) % 2 == 0;
}

bool GlopKey::IsJoystickHat() const {
  return IsJoystickKey() && index >= kJoystickHatStart && index < kJoystickHatEnd;
}

bool GlopKey::IsJoystickHatUp() const {
  return IsJoystickHat() && (index - kJoystickHatUp) % 4 == 0;
}

bool GlopKey::IsJoystickHatRight() const {
  return IsJoystickHat() && (index - kJoystickHatRight) % 4 == 0;
}

bool GlopKey::IsJoystickHatDown() const {
  return IsJoystickHat() && (index - kJoystickHatDown) % 4 == 0;
}

bool GlopKey::IsJoystickHatLeft() const {
  return IsJoystickHat() && (index - kJoystickHatLeft) % 4 == 0;
}

bool GlopKey::IsJoystickButton() const {
  return IsJoystickKey() && index >= kJoystickButtonStart && index < kJoystickButtonEnd;
}

bool GlopKey::IsMotionKey() const {
  return IsMouseMotion() || IsJoystickAxis() || IsJoystickHat();
}

bool GlopKey::IsModifierKey() const {
  return *this == kKeyLeftShift || *this == kKeyRightShift || *this == kKeyLeftControl ||
         *this == kKeyRightControl || *this == kKeyLeftAlt || *this == kKeyRightAlt;
}

int GlopKey::GetJoystickAxisNumber() const {
  return (index - kJoystickAxisStart) / 2;
}

int GlopKey::GetJoystickHatNumber() const {
  return (index - kJoystickHatStart) / 4;
}

int GlopKey::GetJoystickButtonNumber() const {
  return index - kJoystickButtonStart;
}

// KeyListener
// ===========

void KeyListener::BeginKeyListening() {
  if (listener_id_ == 0)
    listener_id_ = input()->key_listeners_.InsertItem(this);
}

void KeyListener::StopKeyListening() {
  if (listener_id_ != 0) {
    input()->key_listeners_.RemoveItem(listener_id_);
    listener_id_ = 0;
  }
}

// Input status
// ============

unsigned char Input::GetAsciiValue(const GlopKey &key) const {
  if (IsKeyDownNow(kKeyLeftAlt) || IsKeyDownNow(kKeyRightAlt) ||
      IsKeyDownNow(kKeyLeftControl) || IsKeyDownNow(kKeyRightControl))
    return 0;
  if (key.index >= kKeyPad0.index && key.index <= kKeyPad9.index && IsNumLockSet())
    return '0' + (key.index - kKeyPad0.index);
  bool is_shift_down = IsKeyDownNow(kKeyLeftShift) || IsKeyDownNow(kKeyRightShift);
  if (key.index >= 'A' && key.index <= 'Z' && !IsCapsLockSet())
    is_shift_down = !is_shift_down;
  return (is_shift_down? kShiftedAsciiValues[key.index] : kAsciiValues[key.index]);
}

void Input::SetMousePosition(int x, int y) {
  Os::SetMousePosition(x + window_x_, y + window_y_);
}

void Input::ShowMouseCursor(bool is_visible) {
  is_cursor_visible_ = is_visible;
  UpdateOsCursorVisibility();
}

const GlopKey &Input::GetKeyPress(bool accept_clicks, bool accept_modifiers, bool accept_motion) {
  for (int i = 0; i < (int)pressed_keys_frame_.size(); i++) {
    const GlopKey &key = pressed_keys_frame_[i];
    if ((accept_clicks || key.IsMotionKey() || !key.IsMouseKey()) &&
        (accept_modifiers || !key.IsModifierKey()) &&
        (accept_motion || !key.IsMotionKey()))
      return key;
  }
  return kNoKey;
}

const GlopKey &Input::WaitForKeyPress(bool accept_clicks, bool accept_modifiers,
                                      bool accept_motion) {
  while (1) {
    gSystem->Think();
    const GlopKey &key = GetKeyPress(accept_clicks, accept_modifiers, accept_motion);
    if (key != kNoKey)
      return key;
  }
}

// GlopWindow interface
// ====================

Input::Input(GlopWindow *window)
: window_(window),
  polling_thread_(new InputPollingThread(&window->os_data_)),
  last_poll_time_(0),
  window_x_(-1),
  window_y_(-1),
  mouse_sensitivity_(1), 
  mouse_x_(0),
  mouse_y_(0),
  is_cursor_visible_(true),
  os_is_cursor_visible_(true),
  num_joysticks_(0),
  joystick_refresh_time_(kJoystickRefreshDelay),
  requested_joystick_refresh_(true) {}

Input::~Input() {
  delete polling_thread_;
}

void Input::StartPolling() {
  polling_thread_->Start();
}

void Input::StopPolling() {
  polling_thread_->RequestStop();
  polling_thread_->Join();
}

// Performs all per-frame logic for the input manager. lost_focus indicates whether the owning
// window either lost input focus or was recreated during the last frame. In either case, we need
// to forget all key down information because it may no longer be current.
void Input::Think(bool lost_focus, int frame_dt) {
  // Update mouse and joystick status
  UpdateOsCursorVisibility();
  if (joystick_refresh_time_ < kJoystickRefreshDelay) {
    joystick_refresh_time_ += frame_dt;
  } else if (requested_joystick_refresh_) {
    polling_thread_->RefreshJoysticks();
    int new_num_joysticks = Os::GetNumJoysticks(window_->os_data_);
    if (num_joysticks_ != new_num_joysticks) {
      lost_focus = true;
      num_joysticks_ = new_num_joysticks;
      joystick_state_.clear();
      joystick_state_.resize(num_joysticks_, vector<KeyInfo>(kJoystickNumKeys));
    }
    joystick_refresh_time_ = 0;
    requested_joystick_refresh_ = false;
  }

  // What has happened since our last poll? Even if we have gone out of focus, we should still
  // check the polling thread so that the information gets cleared.
  vector<bool> is_num_lock_set, is_caps_lock_set;
  vector<int> time;
  vector<int> cursor_x, cursor_y;
  vector<int> mouse_dx, mouse_dy;
  vector<vector<Os::KeyEvent> > os_events;
  os_events = polling_thread_->GetData(&time, &is_num_lock_set, &is_caps_lock_set, &cursor_x,
                                       &cursor_y, &mouse_dx, &mouse_dy); 
  int n = (int)os_events.size();
  Os::GetWindowPosition(window_->os_data_, &window_x_, &window_y_);
  for (int i = 0; i < n; i++) {
    cursor_x[i] -= window_x_;
    cursor_y[i] -= window_y_;
  }

  // If we have lost focus, clear all key state. Note that down_keys_frame_ is rebuilt every frame
  // regardless, so we do not need to worry about it here.
  if (lost_focus) {
    repeat_events_.Clear();
    is_num_lock_set_ = is_num_lock_set[n - 1];
    is_caps_lock_set_ = is_caps_lock_set[n - 1];
    mouse_x_ = cursor_x[n - 1];
    mouse_y_ = cursor_y[n - 1];
    for (int i = -1; i < GetNumJoysticks(); i++) {
      int num_keys = (i == -1? kNumKeys : kJoystickNumKeys);
      for (int j = 0; j < num_keys; j++) {
        KeyInfo *info = LocateKeyInfo(GlopKey(i, j));
        if (info->is_down_now) {
          info->press_amount_now = 0;
          OnKeyEvent(KeyEvent(GlopKey(i, j), KeyEvent::Release), 0);
        }
      }
    }
  }

  // Forget all key presses from the last frame.
  down_keys_frame_.clear();
  pressed_keys_frame_.clear();
  for (int i = -1; i < GetNumJoysticks(); i++) {
    int num_keys = (i == -1? kNumKeys : kJoystickNumKeys);
    for (int j = 0; j < num_keys; j++) {
      KeyInfo *info = LocateKeyInfo(GlopKey(i, j));
      info->was_pressed = false;
      info->was_pressed_no_repeats = false;
      info->is_down_frame = info->is_down_now;
      info->press_amount_frame = info->press_amount_now;
    }
  }

  // Now update key statuses for this frame. We only do this if !lost_focus. Otherwise, some down
  // key events might have been generated before losing focus, and the corresponding up key event
  // may never happen.
  // Process all OS event phases
  if (!lost_focus)
  for (int i = 0; i < n; i++) {
    LightSetId last_repeat_event_id = repeat_events_.GetLastId();

    // Calculate the time differential for this phase and add mouse motion events
    int new_total_dt = time[i] - last_poll_time_,
        prev_total_dt = (i == 0? 0 : time[i-1] - last_poll_time_),
        this_dt = new_total_dt - prev_total_dt;

    // Update the new settings
    is_num_lock_set_ = is_num_lock_set[i];
    is_caps_lock_set_ = is_caps_lock_set[i];
    mouse_x_ = cursor_x[i];
    mouse_y_ = cursor_y[i];
    OnKeyEvent(KeyEvent(kNoKey, KeyEvent::Nothing), this_dt);

    // Add mouse mouse events - note this requires dt > 0.
    if (this_dt > 0) {
      float mouse_scale = mouse_sensitivity_ * kBaseMouseSensitivity / this_dt;
      os_events[i].push_back(Os::KeyEvent(kMouseUp, -mouse_dy[i] * mouse_scale));
      os_events[i].push_back(Os::KeyEvent(kMouseRight, mouse_dx[i] * mouse_scale));
      os_events[i].push_back(Os::KeyEvent(kMouseDown, mouse_dy[i] * mouse_scale));
      os_events[i].push_back(Os::KeyEvent(kMouseLeft, -mouse_dx[i] * mouse_scale));
    }

    // Process all key up/key down events from this phase
    for (int j = 0; j < (int)os_events[i].size(); j++) {
      // Get the key information and press amount (accounting for the deadzone)
      KeyInfo *info = LocateKeyInfo(os_events[i][j].key);
      float amt = os_events[i][j].press_amount;
      if (os_events[i][j].key.IsJoystickKey())
        amt = (amt - kJoystickAxisThreshold) / (1 - kJoystickAxisThreshold);
      amt = max(amt, 0.0f);

      // Update the key press amount. The frame press amount calculation will overweight this
      // phase if more than one event happens during one phase, but since each phase is so short,
      // this will be very unlikely to matter.
      info->press_amount_frame = (prev_total_dt * info->press_amount_now + this_dt * amt) /
                                  new_total_dt;
      info->press_amount_now = amt;

      // Handle new key presses. We have one nasty special case here. We do not accept mouse
      // motion "presses" in the opposite direction of a currently active mouse motion press.
      // This could happen because mouse motion stays down for a certain length of time even
      // after the motion is no longer happening.
      if (amt > 0 && !info->is_down_now) {
        GlopKey opposite_key = kNoKey;
        if (os_events[i][j].key.IsMouseKey()) {
          if (os_events[i][j].key == kMouseUp) {
            opposite_key = kMouseDown;
          } else if (os_events[i][j].key == kMouseRight) {
            opposite_key = kMouseLeft;
          } else if (os_events[i][j].key == kMouseDown) {
            opposite_key = kMouseUp;
          } else if (os_events[i][j].key == kMouseLeft) {
            opposite_key = kMouseRight;
          }
        }
        if (opposite_key == kNoKey || !LocateKeyInfo(opposite_key)->is_down_now) {
          if (info->is_press_time_valid && info->press_time + kDoublePressThreshold >= time[i]) {
            OnKeyEvent(KeyEvent(os_events[i][j].key, KeyEvent::DoublePress), 0);
            info->is_press_time_valid = false;
          } else {
            OnKeyEvent(KeyEvent(os_events[i][j].key, KeyEvent::Press), 0);
            info->is_press_time_valid = true;
            info->press_time = time[i];
          }
        }
      }

      // Handle key releases for keys that do not have a min down-time
      if (amt == 0 && info->is_down_now && GetMinDownTime(os_events[i][j].key) == 0)
        OnKeyEvent(KeyEvent(os_events[i][j].key, KeyEvent::Release), 0);
    }

    // Update the min down-time on keys
    for (int i = -1; i < GetNumJoysticks(); i++) {
      int num_keys = (i == -1? kNumKeys : kJoystickNumKeys);
      for (int j = 0; j < num_keys; j++) {
        KeyInfo *info = LocateKeyInfo(GlopKey(i, j));
        int delay = GetMinDownTime(GlopKey(i, j));
        if (delay > 0) {
          if (info->press_amount_now > 0) {
            info->release_delay = delay;
          } else if (info->is_down_now) {
            info->release_delay -= this_dt;
            if (info->release_delay <= 0)
              OnKeyEvent(KeyEvent(GlopKey(i, j), KeyEvent::Release), 0);
          }
        }
      }
    }

    // Handle repeat events. Note that repeat events will be generated while we step through
    // this list (and earlier during this phase). We do not update the time on any of these.
    // If a phase were very long for some reason, doing so could lead to an infinite loop.
    for (LightSetId id = repeat_events_.GetFirstId(); id != 0; id = repeat_events_.GetNextId(id)) {
      bool is_last_event = (id == last_repeat_event_id);
      repeat_events_[id].delay -= this_dt;
      if (repeat_events_[id].delay < 0) {
        OnKeyEvent(KeyEvent(repeat_events_[id].key, KeyEvent::RepeatPress), 0);
        id = repeat_events_.RemoveItem(id);
      }
      if (is_last_event)
        break;
    }
  }

  // Fill the down keys vector
  for (int i = -1; i < GetNumJoysticks(); i++) {
    int num_keys = (i == -1? kNumKeys : kJoystickNumKeys);
    for (int j = 0; j < num_keys; j++)
    if (LocateKeyInfo(GlopKey(i, j))->is_down_frame)
      down_keys_frame_.push_back(GlopKey(i, j));
  }

  // Update the time
  last_poll_time_ = time[n - 1];
}

void Input::OnKeyEvent(const KeyEvent &event, int dt) {
  // Handle key presses
  if (event.IsNonRepeatPress()) {
    KeyInfo *info = LocateKeyInfo(event.key);
    info->is_down_now = true;
    info->is_down_frame = true;
    info->was_pressed = true;
    info->was_pressed_no_repeats = true;
    RepeatEvent new_event;
    new_event.key = event.key;
    new_event.delay = kRepeatDelay;
    repeat_events_.InsertItem(new_event);
  }

  // Handle key repeats
  else if (event.IsRepeatPress()) {
    LocateKeyInfo(event.key)->was_pressed = true;
    RepeatEvent new_event;
    new_event.key = event.key;
    new_event.delay = kRepeatRate;
    repeat_events_.InsertItem(new_event);
  }

  // Handle key releases
  else if (event.IsRelease()) {
    LocateKeyInfo(event.key)->is_down_now = false;
    for (LightSetId id = repeat_events_.GetFirstId(); id != 0; id = repeat_events_.GetNextId(id))
      if (repeat_events_[id].key == event.key)
        id = repeat_events_.RemoveItem(id);
  }

  // Pass on events to our listeners
  if (event.IsPress())
    pressed_keys_frame_.push_back(event.key);
  window_->OnKeyEvent(event, dt);
  for (LightSetId id = key_listeners_.GetFirstId(); id != 0; id = key_listeners_.GetNextId(id))
    key_listeners_[id]->OnKeyEvent(event, dt);
}

// Returns a pointer to the KeyInfo for a given key.
const Input::KeyInfo *Input::LocateKeyInfo(const GlopKey &key) const {
  if (key.joystick == -1) {
    ASSERT(key.index >= 0 && key.index < kNumKeys);
    return &keyboard_state_[key.index];
  } else if (key.joystick >= 0 && (int)joystick_state_.size()) {
    ASSERT(key.index >= 0 && key.index < kJoystickNumKeys);
    return &joystick_state_[key.joystick][key.index];
  } else {
    return 0;
  }
}
Input::KeyInfo *Input::LocateKeyInfo(const GlopKey &key) {
  const Input *const_this = (const Input *)this;
  return (Input::KeyInfo*)const_this->LocateKeyInfo(key);
}

// Figures out whether the OS should be displaying or hiding our cursor, and then informs it if
// it is doing the wrong thing.
void Input::UpdateOsCursorVisibility() {
  bool is_in_focus, focus_changed;
  bool os_is_visible = true;
  int width, height;
  if (!is_cursor_visible_) {
    Os::GetWindowFocusState(window_->os_data_, &is_in_focus, &focus_changed);
    Os::GetWindowSize(window_->os_data_, &width, &height);
    os_is_visible = (!is_in_focus || mouse_x_ < 0 || mouse_y_ < 0 ||
                      mouse_x_ >= width || mouse_y_ >= height);
  }
  if (os_is_visible != os_is_cursor_visible_) {
    Os::ShowMouseCursor(os_is_visible);
    os_is_cursor_visible_ = os_is_visible;
  }
}

// Returns the number of milliseconds that we force a key to stay marked as down after being
// pressed. This is useful for mouse motion so we do not continuously generate press and release
// events.
int Input::GetMinDownTime(const GlopKey &key) {
  if (key == kMouseLeft || key == kMouseUp || key == kMouseRight || key == kMouseDown)
    return 50;
  else
    return 0;
}
