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
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, "[", "\\", "]", 0,
  0, "`", "A", "B", "C",
  "D", "E", "F", "G", "H",                                                            // 100
  "I", "J", "K", "L", "M",
  "N", "O", "P", "Q", "R",
  "S", "T", "U", "V", "W",
  "X", "Y", "Z", 0, 0,
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
  0, '=', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, '[', '\\', ']', 0, 0, '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
  'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
  'x', 'y', 'z', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
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
  0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, '{', '|', '}', 0, 0, '`', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
  'X', 'Y', 'Z', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
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

// Globals
vector<string> Input::derived_key_names_;
vector<vector<vector<GlopKey> > > Input::derived_key_bindings_;
vector<vector<vector<bool> > > Input::derived_key_bindings_down_;

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
  if (device == kDeviceKeyboard) {
    return kKeyNames[index + 2];
  } else if (device == kDeviceDerived) {
    return Input::derived_key_names_[index];
  } else {
    string result;
    if (device == kDeviceAnyJoystick)
      result = "Joystick ";
    else
      result = Format("Joystick #%d ", device + 1);
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

bool GlopKey::IsDerivedKey() const {
  return device == kDeviceAnyJoystick || device == kDeviceDerived;
}

bool GlopKey::IsKeyboardKey() const {
  return device == kDeviceKeyboard && index < kFirstMouseKeyIndex && index != kNoKey.index &&
         index != kAnyKey.index;
}

bool GlopKey::IsMouseKey() const {
  return device == kDeviceKeyboard && index >= kFirstMouseKeyIndex;
}

bool GlopKey::IsJoystickKey() const {
  return device != kDeviceKeyboard;
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
  return ((device == kDeviceKeyboard &&
          (index == kKeyLeftShift.index || index == kKeyRightShift.index ||
           index == kKeyLeftControl.index || index == kKeyRightControl.index ||
           index == kKeyLeftAlt.index || index == kKeyRightAlt.index)) ||
          (device == kDeviceDerived &&
          (index == kKeyEitherShift.index || index == kKeyEitherControl.index ||
           index == kKeyEitherAlt.index)));
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

// KeyTracker
// ==========

Input::KeyTracker::KeyTracker(int release_delay)
: press_amount_now_(0), press_amount_frame_(0),
  is_down_now_(false), is_down_frame_(false),
  total_frame_time_(0), 
  release_delay_left_(0), release_delay_(release_delay),
  double_press_time_left_(0),
  repeat_delay_left_(0),
  was_pressed_(false), was_pressed_no_repeats_(false), was_released_(false) {}

KeyEvent::Type Input::KeyTracker::SetPressAmount(float amount) {
  press_amount_now_ = amount;

  // Handle presses
  if (amount > 0) {
    release_delay_left_ = release_delay_;
    if (!is_down_now_) {
      is_down_now_ = true;
      is_down_frame_ = true;
      was_pressed_ = true;
      was_pressed_no_repeats_ = true;
      repeat_delay_left_ = kRepeatDelay;
      if (double_press_time_left_ > 0) {
        double_press_time_left_ = 0;
        return KeyEvent::DoublePress;
      } else {
        double_press_time_left_ = kDoublePressThreshold;
        return KeyEvent::Press;
      }
    }
  }

  // Handle releases
  if (amount == 0 && is_down_now_ && release_delay_ == 0) {
    is_down_now_ = false;
    was_released_ = true;
    return KeyEvent::Release;
  }
  return KeyEvent::Nothing;
}

KeyEvent::Type Input::KeyTracker::Clear() {
  press_amount_now_ = 0;
  if (is_down_now_) {
    is_down_now_ = false;
    was_released_ = true;
    return KeyEvent::Release;
  }
  return KeyEvent::Nothing;
}

KeyEvent::Type Input::KeyTracker::OnKeyEventDt(int dt) {
  if (dt == 0)
    return KeyEvent::Nothing;

  // Update frame info
  is_down_frame_ |= is_down_now_;
  press_amount_frame_ = (press_amount_frame_ * total_frame_time_ + press_amount_now_ * dt) /
                        (total_frame_time_ + dt);
  total_frame_time_ += dt;

  if (is_down_now_) {
    // Handle releases
    if (press_amount_now_ == 0) {
      release_delay_ -= dt;
      if (release_delay_ <= 0) {
        is_down_now_ = false;
        was_released_ = true;
        return KeyEvent::Release;
      }
    }

    // Handle repeat events
    else {
      repeat_delay_left_ -=  dt;
      if (repeat_delay_left_ <= 0) {
        was_pressed_ = true;
        repeat_delay_left_ += kRepeatRate;
        return KeyEvent::RepeatPress;
      }
    }
  }
  if (double_press_time_left_ > 0) {
    double_press_time_left_ -= dt;
  }
  return KeyEvent::Nothing;
}

void Input::KeyTracker::Think() {
  press_amount_frame_ = press_amount_now_;
  total_frame_time_ = 0;
  is_down_frame_ = is_down_now_;
  was_pressed_ =  was_pressed_no_repeats_ = was_released_ = false;
}

// Input status
// ============

unsigned char Input::GetAsciiValue(const GlopKey &key) const {
  if (!key.IsKeyboardKey() || IsKeyDownNow(kKeyEitherAlt) || IsKeyDownNow(kKeyEitherControl))
    return 0;
  if (key.index >= kKeyPad0.index && key.index <= kKeyPad9.index && IsNumLockSet())
    return '0' + (key.index - kKeyPad0.index);
  bool is_shift_down = IsKeyDownNow(kKeyEitherShift);
  if (key.index >= 'a' && key.index <= 'z' && IsCapsLockSet())
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
        (accept_motion || !key.IsMotionKey()) &&
        !key.IsDerivedKey())
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

void Input::ConfigureGuiKeys(bool keyboard_bindings, bool mouse_bindings, bool joystick_bindings) {
  for (int i = 3; i < kNumBasicDerivedKeys; i++)
    UnbindDerivedKey(GlopKey(i, kDeviceDerived));
  if (keyboard_bindings) {
    BindDerivedKey(kGuiKeyPageUp, kKeyPageUp);
    BindDerivedKey(kGuiKeyPageDown, kKeyPageDown);
    BindDerivedKey(kGuiKeyUp, kKeyUp);
    BindDerivedKey(kGuiKeyRight, kKeyRight);
    BindDerivedKey(kGuiKeyDown, kKeyDown);
    BindDerivedKey(kGuiKeyLeft, kKeyLeft);
    BindDerivedKey(kGuiKeyConfirm, kKeyEnter);
    BindDerivedKey(kGuiKeyConfirm, kKeyPadEnter);
    BindDerivedKey(kGuiKeyCancel, kKeyEscape);
    BindDerivedKey(kGuiKeySelectPrev, kKeyTab, kKeyEitherShift, kKeyEitherAlt, true, true, false);
    BindDerivedKey(kGuiKeySelectNext, kKeyTab, kKeyEitherShift, kKeyEitherAlt, true, false, false);
  }
  if (mouse_bindings) {
    BindDerivedKey(kGuiKeyScrollUp, kMouseWheelUp);
    BindDerivedKey(kGuiKeyScrollDown, kMouseWheelDown);
    BindDerivedKey(kGuiKeyPrimaryClick, kMouseLButton);
    BindDerivedKey(kGuiKeySecondaryClick, kMouseRButton);
  }
  if (joystick_bindings) {
    BindDerivedKey(kGuiKeyUp, GetJoystickUp());
    BindDerivedKey(kGuiKeyRight, GetJoystickRight());
    BindDerivedKey(kGuiKeyDown, GetJoystickDown());
    BindDerivedKey(kGuiKeyLeft, GetJoystickLeft());
    BindDerivedKey(kGuiKeyConfirm, GetJoystickButton(0));
    BindDerivedKey(kGuiKeySelectPrev, GetJoystickUp());
    BindDerivedKey(kGuiKeySelectPrev, GetJoystickLeft());
    BindDerivedKey(kGuiKeySelectPrev, GetJoystickAxisNeg(2));
    BindDerivedKey(kGuiKeySelectPrev, GetJoystickAxisNeg(3));
    BindDerivedKey(kGuiKeySelectNext, GetJoystickDown());
    BindDerivedKey(kGuiKeySelectNext, GetJoystickRight());
    BindDerivedKey(kGuiKeySelectNext, GetJoystickAxisPos(2));
    BindDerivedKey(kGuiKeySelectNext, GetJoystickAxisPos(3));
  }
}

GlopKey Input::AllocateDerivedKey(const string &key_name) {
  derived_key_names_.push_back(key_name);
  derived_key_bindings_.push_back(vector<vector<GlopKey> >(0));
  derived_key_bindings_down_.push_back(vector<vector<bool> >(0));
  return GlopKey(GetNumDerivedKeys() - 1, kDeviceDerived);
}

void Input::UnbindDerivedKey(const GlopKey &derived_key) {
  int index = derived_key.index;
  ASSERT(derived_key.device == kDeviceDerived);
  ASSERT(index >= kNumFixedDerivedKeys && index < GetNumDerivedKeys());
  derived_key_bindings_[index].clear();
  derived_key_bindings_down_[index].clear();
}

void Input::BindDerivedKey(const GlopKey &derived_key, const GlopKey &binding, bool down) {
  Input::BindDerivedKey(derived_key, vector<GlopKey>(1, binding), vector<bool>(1, down));
}

void Input::BindDerivedKey(const GlopKey &derived_key, const GlopKey &binding1,
                           const GlopKey &binding2, bool down1, bool down2) {
  vector<GlopKey> binding;
  binding.push_back(binding1);
  binding.push_back(binding2);
  vector<bool> down;
  down.push_back(down1);
  down.push_back(down2);
  BindDerivedKey(derived_key, binding, down);
}

void Input::BindDerivedKey(const GlopKey &derived_key, const GlopKey &binding1,
                           const GlopKey &binding2, const GlopKey &binding3, bool down1, bool down2,
                           bool down3) {
  vector<GlopKey> binding;
  binding.push_back(binding1);
  binding.push_back(binding2);
  binding.push_back(binding3);
  vector<bool> down;
  down.push_back(down1);
  down.push_back(down2);
  down.push_back(down3);
  BindDerivedKey(derived_key, binding, down);
}

void Input::BindDerivedKey(const GlopKey &derived_key, const vector<GlopKey> &binding,
                           const vector<bool> &down) {
  int index = derived_key.index;
  ASSERT(derived_key.device == kDeviceDerived);
  ASSERT(index >= kNumFixedDerivedKeys && index < GetNumDerivedKeys());
  ASSERT(binding.size() == down.size());
  for (int i = 0; i < (int)binding.size(); i++)
    ASSERT(binding[i].device != kDeviceDerived || binding[i].index < index);
  derived_key_bindings_[index].push_back(binding);
  derived_key_bindings_down_[index].push_back(down);
}

void Input::ClearDerivedKeys() {
  derived_key_names_.resize(kNumBasicDerivedKeys);
  derived_key_bindings_.resize(kNumBasicDerivedKeys);
  derived_key_bindings_down_.resize(kNumBasicDerivedKeys);
}

void Input::InitDerivedKeys() {
 // Allocate the derived keys
  ASSERT(kKeyEitherShift == AllocateDerivedKey("Shift"));
  ASSERT(kKeyEitherControl == AllocateDerivedKey("Control"));
  ASSERT(kKeyEitherAlt == AllocateDerivedKey("Alt"));
  ASSERT(kGuiKeyPageUp == AllocateDerivedKey("Gui page up"));
  ASSERT(kGuiKeyPageRight == AllocateDerivedKey("Gui page right"));
  ASSERT(kGuiKeyPageDown == AllocateDerivedKey("Gui page down"));
  ASSERT(kGuiKeyPageLeft == AllocateDerivedKey("Gui page left"));
  ASSERT(kGuiKeyScrollUp == AllocateDerivedKey("Gui scroll up"));
  ASSERT(kGuiKeyScrollRight == AllocateDerivedKey("Gui scroll right"));
  ASSERT(kGuiKeyScrollDown == AllocateDerivedKey("Gui scroll down"));
  ASSERT(kGuiKeyScrollLeft == AllocateDerivedKey("Gui scroll left"));
  ASSERT(kGuiKeyUp == AllocateDerivedKey("Gui up"));
  ASSERT(kGuiKeyRight == AllocateDerivedKey("Gui right"));
  ASSERT(kGuiKeyDown == AllocateDerivedKey("Gui down"));
  ASSERT(kGuiKeyLeft == AllocateDerivedKey("Gui left"));
  ASSERT(kGuiKeyConfirm == AllocateDerivedKey("Gui confirm"));
  ASSERT(kGuiKeyCancel == AllocateDerivedKey("Gui cancel"));
  ASSERT(kGuiKeyPrimaryClick == AllocateDerivedKey("Gui primary click"));
  ASSERT(kGuiKeySecondaryClick == AllocateDerivedKey("Gui secondary click"));
  ASSERT(kGuiKeySelectNext == AllocateDerivedKey("Gui select next"));
  ASSERT(kGuiKeySelectPrev == AllocateDerivedKey("Gui select prev"));
  ASSERT(kGuiKeySelectPrev.index == kNumBasicDerivedKeys - 1);

  // Bind the derived keys
  derived_key_bindings_[kKeyEitherShift.index].push_back(vector<GlopKey>(1, kKeyLeftShift));
  derived_key_bindings_[kKeyEitherShift.index].push_back(vector<GlopKey>(1, kKeyRightShift));
  derived_key_bindings_[kKeyEitherControl.index].push_back(vector<GlopKey>(1, kKeyLeftControl));
  derived_key_bindings_[kKeyEitherControl.index].push_back(vector<GlopKey>(1, kKeyRightControl));
  derived_key_bindings_[kKeyEitherAlt.index].push_back(vector<GlopKey>(1, kKeyLeftAlt));
  derived_key_bindings_[kKeyEitherAlt.index].push_back(vector<GlopKey>(1, kKeyRightAlt));
  derived_key_bindings_down_[kKeyEitherShift.index] =
    vector<vector<bool> >(2, vector<bool>(1, true));
  derived_key_bindings_down_[kKeyEitherControl.index] =
    vector<vector<bool> >(2, vector<bool>(1, true));
  derived_key_bindings_down_[kKeyEitherAlt.index] =
    vector<vector<bool> >(2, vector<bool>(1, true));
  ConfigureGuiKeys(true, true, false);
}

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
  requested_joystick_refresh_(true) {
  GetKeyTracker(kMouseUp)->SetReleaseDelay(100);
  GetKeyTracker(kMouseRight)->SetReleaseDelay(100);
  GetKeyTracker(kMouseDown)->SetReleaseDelay(100);
  GetKeyTracker(kMouseLeft)->SetReleaseDelay(100);
}

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
  // Update all editable derived keys - this is useful in the case that the user has changed their
  // definitions since the last frame.
  derived_key_trackers_.resize(GetNumDerivedKeys());
  for (int i = 0; i < GetNumDerivedKeys(); i++)
    UpdateDerivedKey(GlopKey(i, kDeviceDerived));

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
      joystick_key_trackers_.clear();
      joystick_key_trackers_.resize(num_joysticks_, vector<KeyTracker>(kNumJoystickKeys));
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
    is_num_lock_set_ = is_num_lock_set[n - 1];
    is_caps_lock_set_ = is_caps_lock_set[n - 1];
    mouse_x_ = cursor_x[n - 1];
    mouse_y_ = cursor_y[n - 1];
    for (int i = kMinDevice; i <= GetMaxDevice(); i++)
    for (int j = 0; j < GetNumKeys(i); j++)
    if (GetKeyTracker(GlopKey(j, i))->Clear() == KeyEvent::Release)
      OnKeyEvent(KeyEvent(GlopKey(j, i), KeyEvent::Release), 0);
  }

  // Do all per-frame logic on keys.
  down_keys_frame_.clear();
  pressed_keys_frame_.clear();
  for (int i = kMinDevice; i <= GetMaxDevice(); i++)
  for (int j = 0; j < GetNumKeys(i); j++)
    GetKeyTracker(GlopKey(j, i))->Think();

  // Now update key statuses for this frame. We only do this if !lost_focus. Otherwise, some down
  // key events might have been generated before losing focus, and the corresponding up key event
  // may never happen.
  // Process all OS event phases
  if (!lost_focus)
  for (int i = 0; i < n; i++) {
    // Calculate the time differential for this phase and add mouse motion events
    int this_dt = time[i] - (i == 0? last_poll_time_ : time[i-1]);

    // Update the new settings
    is_num_lock_set_ = is_num_lock_set[i];
    is_caps_lock_set_ = is_caps_lock_set[i];
    mouse_x_ = cursor_x[i];
    mouse_y_ = cursor_y[i];

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
      ASSERT(!os_events[i][j].key.IsDerivedKey());

      // Get the key information and press amount (accounting for the deadzone)
      KeyTracker *info = GetKeyTracker(os_events[i][j].key);
      float amt = os_events[i][j].press_amount;
      if (os_events[i][j].key.IsJoystickKey())
        amt = (amt - kJoystickAxisThreshold) / (1 - kJoystickAxisThreshold);
      amt = max(amt, 0.0f);
      KeyEvent::Type type = info->SetPressAmount(amt);
      if (type != KeyEvent::Nothing)
        OnKeyEvent(KeyEvent(os_events[i][j].key, type), 0);

      // Update derived keys that could possibly be affected
      if (os_events[i][j].key.IsJoystickKey()) {
        if (os_events[i][j].key.IsJoystickAxis()) {
          UpdateDerivedKey(GetJoystickAxisPos(os_events[i][j].key.GetJoystickAxisNumber()));
          UpdateDerivedKey(GetJoystickAxisNeg(os_events[i][j].key.GetJoystickAxisNumber()));
        } else {
          UpdateDerivedKey(GlopKey(os_events[i][j].key.index, kDeviceAnyJoystick));
        }
      }
      for (int k = 0; k < GetNumDerivedKeys(); k++)
        UpdateDerivedKey(GlopKey(k, kDeviceDerived));
    }

    // Send elapsed time messages
    for (int j = kMinDevice; j <= GetMaxDevice(); j++)
    for (int k = 0; k < GetNumKeys(j); k++) {
      KeyTracker *info = GetKeyTracker(GlopKey(k, j));
      KeyEvent::Type type = info->OnKeyEventDt(this_dt);
      if (type != KeyEvent::Nothing)
        OnKeyEvent(KeyEvent(GlopKey(k, j), type), 0);
    }
    OnKeyEvent(KeyEvent(kNoKey, KeyEvent::Nothing), this_dt);
  }

  // Fill the down keys vector
  for (int i = kMinDevice; i <= GetMaxDevice(); i++)
  for (int j = 0; j < GetNumKeys(i); j++)
  if (GetKeyTracker(GlopKey(j, i))->IsDownFrame())
    down_keys_frame_.push_back(GlopKey(j, i));

  // Update the time
  last_poll_time_ = time[n - 1];
}

// Recalculates how pressed a derived key is, and then updates our internal records, possibly
// sending out event notifications.
void Input::UpdateDerivedKey(const GlopKey &key) {
  float amount = 0.0f;

  // Get press amount for normal derived keys
  if (key.device == kDeviceDerived) {
    for (int i = 0; i < (int)derived_key_bindings_[key.index].size(); i++) {
      bool is_binding_down = true;
      for (int j = 0; j < (int)derived_key_bindings_[key.index][i].size(); j++)
      if (IsKeyDownNow(derived_key_bindings_[key.index][i][j]) !=
          derived_key_bindings_down_[key.index][i][j])
        is_binding_down = false;
      if (is_binding_down) {
        amount = 1.0f;
        break;
      }
    }
  }
  
  // Get press amount for any-joystick derived keys
  else {
    ASSERT(key.device == kDeviceAnyJoystick);
    if (key.IsJoystickAxis()) {
      int axis = key.GetJoystickAxisNumber();
      for (int i = 0; i < GetNumJoysticks(); i++)
        amount += GetKeyPressAmountNow(GetJoystickAxisPos(axis, i)) -
                  GetKeyPressAmountNow(GetJoystickAxisNeg(axis, i));
      if (key.IsJoystickAxisNeg()) amount = -amount;
      amount = max(amount, 0.0f);
    } else {
      for (int i = 0; i < GetNumJoysticks(); i++)
        amount = max(amount, GetKeyPressAmountNow(GlopKey(key.index, i)));
    }
  }

  // Update the key
  KeyEvent::Type type = GetKeyTracker(key)->SetPressAmount(amount);
  if (type != KeyEvent::Nothing)
    OnKeyEvent(KeyEvent(key, type), 0);
}

// Updates internal records and sends out notifications on a key event.
void Input::OnKeyEvent(const KeyEvent &event, int dt) {
  if (event.IsPress())
    pressed_keys_frame_.push_back(event.key);
  window_->OnKeyEvent(event, dt);
  for (LightSetId id = key_listeners_.GetFirstId(); id != 0; id = key_listeners_.GetNextId(id))
    key_listeners_[id]->OnKeyEvent(event, dt);
}

// Returns a pointer to the KeyTracker corresponding to a given GlopKey.
const Input::KeyTracker *Input::GetKeyTracker(const GlopKey &key) const {
  if (key.device == kDeviceKeyboard) {
    ASSERT(key.index >= 0 && key.index < kNumKeyboardKeys);
    return &keyboard_key_trackers_[key.index];
  } else if (key.device >= 0 && key.device < (int)joystick_key_trackers_.size()) {
    ASSERT(key.index >= 0 && key.index < kNumJoystickKeys);
    return &joystick_key_trackers_[key.device][key.index];
  } else if (key.device == kDeviceAnyJoystick) {
    ASSERT(key.index >= 0 && key.index < kNumJoystickKeys);
    return &any_joystick_key_trackers_[key.index];
  } else if (key.device == kDeviceDerived) {
    ASSERT(key.index >= 0 && key.index < GetNumDerivedKeys());
    return &derived_key_trackers_[key.index];
  } else {
    return 0;
  }
}
Input::KeyTracker *Input::GetKeyTracker(const GlopKey &key) {
  const Input *const_this = (const Input *)this;
  return (KeyTracker*)const_this->GetKeyTracker(key);
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
