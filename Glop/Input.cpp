// Includes
#include "Input.h"
#include "GlopWindow.h"
#include "Os.h"
#include "System.h"
#include "Thread.h"

// Constants
const float kBaseMouseSensitivity = 3.0f;
const float kJoystickAxisThreshold = 0.2f;
const int kDoublePressThreshold = -1;
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
  "D", "E", "F", "G", "H",                                                  // 100
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
  "Right alt", "Left gui", "Right gui", 0, 0,
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
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  "Mouse up", "Mouse right", "Mouse down", "Mouse left", "Mouse wheel up",  // 300
  "Mouse wheel down", "Left mouse button", "Right mouse button",
  "Middle mouse button", "Mouse button #4",
  "Mouse button #5", "Mouse button #6", "Mouse button #7", "Mouse button #8"};
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
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
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
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Globals
vector<string> Input::derived_key_names_;
vector<vector<Input::DerivedKeyBinding> > Input::derived_key_bindings_;

// Static accessor convenience method
Input *input() {
  return system()->window()->input();
}

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
    listener_id_ = input()->key_listeners_.push_back(this);
}

void KeyListener::StopKeyListening() {
  if (listener_id_ != 0) {
    input()->key_listeners_.erase(listener_id_);
    listener_id_ = 0;
  }
}

// KeyState
// ========

Input::KeyState::KeyState()
: press_amount_now_(0), press_amount_frame_(0),
  is_down_now_(false), is_down_frame_(false),
  total_frame_time_(0), double_press_time_left_(0), 
  was_pressed_(false), was_pressed_no_repeats_(false), was_released_(false) {}

KeyEvent::Type Input::KeyState::SetIsDown(bool is_down, bool generate_press_events) {
  if (is_down == is_down_now_)
    return KeyEvent::Nothing;
  is_down_now_ = is_down;
  is_down_frame_ |= is_down;
  if (!is_down) {
    was_released_ = true;
    return KeyEvent::Release;
  }
  if (generate_press_events) {
    was_pressed_ = true;
    if (double_press_time_left_ > 0) {
      double_press_time_left_ = 0;
      return KeyEvent::DoublePress;
    } else {
      double_press_time_left_ = kDoublePressThreshold;
      return KeyEvent::Press;
    }
  } else {
    return KeyEvent::Nothing;
  }
}

void Input::KeyState::OnDt(int dt) {
  press_amount_frame_ = (press_amount_frame_ * total_frame_time_ + press_amount_now_ * dt) /
                        (total_frame_time_ + dt);
  total_frame_time_ += dt;
  if (double_press_time_left_ > 0)
    double_press_time_left_ -= dt;
}

void Input::KeyState::Think() {
  press_amount_frame_ = press_amount_now_;
  total_frame_time_ = 0;
  is_down_frame_ = is_down_now_;
  was_pressed_ = was_pressed_no_repeats_ = was_released_ = false;
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

void Input::LockMouseCursor(bool is_locked) {
  is_cursor_locked_ = is_locked;
  Os::LockMouseCursor(is_locked);
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
  // Note that we need to think before checking for key presses. This ensures we don't exit
  // immediately if a key was already pressed this frame.
  system()->Think();
  while (1) {
    system()->Think();
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
    BindDerivedKey(kGuiKeyPageUp, kKeyPad9);
    BindDerivedKey(kGuiKeyPageDown, kKeyPageDown);
    BindDerivedKey(kGuiKeyPageDown, kKeyPad3);
    BindDerivedKey(kGuiKeyUp, kKeyUp);
    BindDerivedKey(kGuiKeyUp, kKeyPad8);
    BindDerivedKey(kGuiKeyRight, kKeyRight);
    BindDerivedKey(kGuiKeyRight, kKeyPad6);
    BindDerivedKey(kGuiKeyDown, kKeyDown);
    BindDerivedKey(kGuiKeyDown, kKeyPad2);
    BindDerivedKey(kGuiKeyLeft, kKeyLeft);
    BindDerivedKey(kGuiKeyLeft, kKeyPad4);
    BindDerivedKey(kGuiKeyConfirm, kKeyEnter);
    BindDerivedKey(kGuiKeyConfirm, kKeyPadEnter);
    BindDerivedKey(kGuiKeyCancel, kKeyEscape);
    BindDerivedKey(kGuiKeySelectPrev, kKeyTab, kKeyEitherShift, kKeyEitherAlt, true, false);
    BindDerivedKey(kGuiKeySelectNext, kKeyTab, kKeyEitherShift, kKeyEitherAlt, false, false);
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
  derived_key_bindings_.push_back(vector<DerivedKeyBinding>(0));
  return GlopKey(GetNumDerivedKeys() - 1, kDeviceDerived);
}

void Input::UnbindDerivedKey(const GlopKey &derived_key) {
  int index = derived_key.index;
  ASSERT(derived_key.device == kDeviceDerived);
  ASSERT(index >= kNumFixedDerivedKeys && index < GetNumDerivedKeys());
  derived_key_bindings_[index].clear();
}

void Input::BindDerivedKey(const GlopKey &derived_key, const GlopKey &key) {
  Input::BindDerivedKey(derived_key, key, vector<GlopKey>(0), vector<bool>(0));
}

void Input::BindDerivedKey(const GlopKey &derived_key, const GlopKey &key,
                           const GlopKey &modifier, bool down) {
  BindDerivedKey(derived_key, key, vector<GlopKey>(1, modifier), vector<bool>(1, down));
}

void Input::BindDerivedKey(const GlopKey &derived_key, const GlopKey &key,
                           const GlopKey &modifier1, const GlopKey &modifier2,
                           bool down1, bool down2) {
  vector<GlopKey> modifiers;
  modifiers.push_back(modifier1);
  modifiers.push_back(modifier2);
  vector<bool> down;
  down.push_back(down1);
  down.push_back(down2);
  BindDerivedKey(derived_key, key, modifiers, down);
}

void Input::BindDerivedKey(const GlopKey &derived_key, const GlopKey &key,
                           const vector<GlopKey> &modifiers, const vector<bool> &down) {
  int index = derived_key.index;
  ASSERT(derived_key.device == kDeviceDerived);
  ASSERT(index >= kNumFixedDerivedKeys && index < GetNumDerivedKeys());
  ASSERT(modifiers.size() == down.size());
  for (int i = 0; i < (int)modifiers.size(); i++)
    ASSERT(modifiers[i].device != kDeviceDerived || modifiers[i].index < index);
  derived_key_bindings_[index].push_back(DerivedKeyBinding(key, modifiers, down));
}

void Input::ClearDerivedKeys() {
  derived_key_names_.resize(kNumBasicDerivedKeys);
  derived_key_bindings_.resize(kNumBasicDerivedKeys);
}

// KeyTracker
// ==========

Input::KeyTracker::KeyTracker()
: requested_press_amount_(0), release_delay_left_(0), release_delay_(0),
  repeat_delay_left_(0) {}

KeyEvent::Type Input::KeyTracker::SetPressAmount(float amount) {
  requested_press_amount_ = amount;

  // Handle presses
  if (amount > 0) {
    state_.SetPressAmount(amount);
    release_delay_left_ = release_delay_;
    if (!state_.IsDownNow()) {
      repeat_delay_left_ = kRepeatDelay;
      return state_.SetIsDown(true, true);
    }

    // For the mouse wheel, we hold the press amount down for a period of time to make it function
    // similar to other keys for smooth movement. However, tracking presses is done much better by
    // just sending events directly from the Os.
    if (mouse_wheel_hack_)
      return KeyEvent::RepeatPress;
  }

  // Handle releases
  else if (state_.IsDownNow()) {
    if (release_delay_ == 0) {
      state_.SetPressAmount(0);
      return state_.SetIsDown(false, true);
    } else if (!mouse_wheel_hack_) {
      state_.SetPressAmount(0);
    }
  }
  return KeyEvent::Nothing;
}

KeyEvent::Type Input::KeyTracker::Clear() {
  state_.SetPressAmount(0);
  return state_.SetIsDown(false, true);
}

KeyEvent::Type Input::KeyTracker::OnDt(int dt) {
  if (dt == 0)
    return KeyEvent::Nothing;
  state_.OnDt(dt);

  if (state_.IsDownNow()) {
    // Handle releases
    if (requested_press_amount_ == 0) {
      release_delay_left_ -= dt;
      if (release_delay_left_ <= 0) {
        state_.SetPressAmount(0);
        return state_.SetIsDown(false, true);
      }
    }

    // Handle repeat events
    if (state_.IsDownNow() && !mouse_wheel_hack_) {
      repeat_delay_left_ -= dt;
      if (repeat_delay_left_ <= 0) {
        state_.OnKeyEvent(KeyEvent::RepeatPress);
        repeat_delay_left_ += kRepeatRate;
        return KeyEvent::RepeatPress;
      }
    }
  }
  return KeyEvent::Nothing;
}

// Private member functions
// ========================

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
  derived_key_bindings_[kKeyEitherShift.index].push_back(DerivedKeyBinding(kKeyLeftShift));
  derived_key_bindings_[kKeyEitherShift.index].push_back(DerivedKeyBinding(kKeyRightShift));
  derived_key_bindings_[kKeyEitherControl.index].push_back(DerivedKeyBinding(kKeyLeftControl));
  derived_key_bindings_[kKeyEitherControl.index].push_back(DerivedKeyBinding(kKeyRightControl));
  derived_key_bindings_[kKeyEitherAlt.index].push_back(DerivedKeyBinding(kKeyLeftAlt));
  derived_key_bindings_[kKeyEitherAlt.index].push_back(DerivedKeyBinding(kKeyRightAlt));
  ConfigureGuiKeys(true, true, false);
}

Input::Input(GlopWindow *window)
: window_(window),
  last_poll_time_set_(false),
  window_x_(-1), window_y_(-1),
  mouse_sensitivity_(1), 
  mouse_x_(0), mouse_y_(0),
  mouse_dx_(0), mouse_dy_(0),
  is_cursor_visible_(true),
  os_is_cursor_visible_(true),
  num_joysticks_(0),
  joystick_refresh_time_(kJoystickRefreshDelay),
  requested_joystick_refresh_(true) {
  GetNonDerivedKeyTracker(kMouseUp)->SetReleaseDelay(100, false);
  GetNonDerivedKeyTracker(kMouseRight)->SetReleaseDelay(100, false);
  GetNonDerivedKeyTracker(kMouseDown)->SetReleaseDelay(100, false);
  GetNonDerivedKeyTracker(kMouseLeft)->SetReleaseDelay(100, false);
  GetNonDerivedKeyTracker(kMouseWheelUp)->SetReleaseDelay(150, true);
  GetNonDerivedKeyTracker(kMouseWheelDown)->SetReleaseDelay(150, true);
}

// Performs all per-frame logic for the input manager. lost_focus indicates whether the owning
// window either lost input focus or was recreated during the last frame. In either case, we need
// to forget all key down information because it may no longer be current.
void Input::Think(bool lost_focus, int frame_dt) {
  // Update all editable derived keys - this is useful in the case that the user has changed their
  // definitions since the last frame. We do it here rather than at key-binding time since the data
  // here is naturally tied to a single window, but key binding is naturally static across all
  // windows.
  derived_key_states_.resize(GetNumDerivedKeys());
  for (int i = 0; i < GetNumDerivedKeys(); i++) {
    GlopKey key(i, kDeviceDerived);
    vector<GlopKey> released_keys;
    UpdateDerivedKeyState(key, &released_keys);
    if (released_keys.size() > 0)
      OnKeyEvent(KeyEvent(released_keys, KeyEvent::Release));
  }

  // Update mouse and joystick status. If the number of joysticks changes, we do a full reset of
  // all input data.
  UpdateOsCursorVisibility();
  if (joystick_refresh_time_ < kJoystickRefreshDelay) {
    joystick_refresh_time_ += frame_dt;
  } else if (requested_joystick_refresh_) {
    Os::RefreshJoysticks(window_->os_data_);
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

  // What has happened since our last poll? Even if we have gone out of focus, we still promise to
  // call Os::GetInputEvents.
  vector<Os::KeyEvent> os_events = Os::GetInputEvents(window_->os_data_);
  Os::GetWindowPosition(window_->os_data_, &window_x_, &window_y_);
  int n = (int)os_events.size();
  ASSERT(n > 0);

  // If we have lost focus, clear all key state. Note that down_keys_frame_ is rebuilt every frame
  // regardless, so we do not need to worry about it here.
  if (lost_focus) {
    // Set the current state
    is_num_lock_set_ = os_events[n - 1].is_num_lock_set;
    is_caps_lock_set_ = os_events[n - 1].is_caps_lock_set;
    mouse_x_ = os_events[n - 1].cursor_x - window_x_;
    mouse_y_ = os_events[n - 1].cursor_y - window_y_;

    // Release all keys. Releasing each non-derived key will also release the derived keys, but
    // we need to explicitly recalculate the press amount (to 0) for derived keys.
    for (int i = kMinDevice; i <= GetMaxDevice(); i++)
    for (int j = 0; j < GetNumKeys(i); j++)
    if (!GlopKey(j, i).IsDerivedKey()) {
      if (GetNonDerivedKeyTracker(GlopKey(j, i))->Clear() == KeyEvent::Release)
        UpdateDerivedKeyStatesAndProcessEvents(GlopKey(j, i), KeyEvent::Release);
    }
  }

  // Do all per-frame logic on keys.
  down_keys_frame_.clear();
  pressed_keys_frame_.clear();
  for (int i = kMinDevice; i <= GetMaxDevice(); i++)
  for (int j = 0; j < GetNumKeys(i); j++)
  if (GlopKey(j, i).IsDerivedKey())
    GetKeyState(GlopKey(j, i))->Think();
  else
    GetNonDerivedKeyTracker(GlopKey(j, i))->Think();

  // Now update key statuses for this frame. We only do this if !lost_focus. Otherwise, some down
  // key os_events might have been generated before losing focus, and the corresponding up key
  // event may never happen.
  if (!lost_focus)
  for (int i = 0; i < n; i++) {
    // Calculate the time differential for this phase and add mouse motion os_events. Note that we
    // do not assume the times are strictly increasing. This is because the dummy input event is
    // likely generated asynchronously from the other os_events, and could thus cause an
    // inconsistent ordering.
    const int kTimeGranularity = 10;
    int new_t = os_events[i].timestamp;
    int old_t = (i == 0? last_poll_time_ : os_events[i-1].timestamp);
    
    // Handle the case where last_poll_time_ is not yet initialized.
    if (!last_poll_time_set_) {
      last_poll_time_ = old_t = new_t;
      last_poll_time_set_ = true;
    }
    int t_boundary = ((old_t + kTimeGranularity - 1) / kTimeGranularity) * kTimeGranularity;

    // Update last_poll_time_, accounting for both overflow and for the fact that new_t may be
    // less than last_poll_time_.
    if ( (new_t - last_poll_time_) >= 0)
      last_poll_time_ = new_t;
    for (int t = t_boundary; (t - new_t) < 0; t += kTimeGranularity) {
      // Send elapsed time messages
      for (int j = kMinDevice; j <= GetMaxDevice(); j++)
      for (int k = 0; k < GetNumKeys(j); k++)
      if (GlopKey(k, j).IsDerivedKey()) {
        GetKeyState(GlopKey(k, j))->OnDt(kTimeGranularity);
      } else {
        KeyTracker *info = GetNonDerivedKeyTracker(GlopKey(k, j));
        KeyEvent::Type type = info->OnDt(kTimeGranularity);
        if (type != KeyEvent::Nothing)
          UpdateDerivedKeyStatesAndProcessEvents(GlopKey(k, j), type);
      }
      float mouse_scale = mouse_sensitivity_ * kBaseMouseSensitivity / kTimeGranularity;
      SetNonDerivedKeyPressAmount(kMouseUp, -mouse_dy_ * mouse_scale);
      SetNonDerivedKeyPressAmount(kMouseRight, mouse_dx_ * mouse_scale);
      SetNonDerivedKeyPressAmount(kMouseDown, mouse_dy_ * mouse_scale);
      SetNonDerivedKeyPressAmount(kMouseLeft, -mouse_dx_ * mouse_scale);
      mouse_dx_ = mouse_dy_ = 0;
      OnKeyEvent(KeyEvent(kTimeGranularity));
    }

    // Update the new settings
    is_num_lock_set_ = os_events[i].is_num_lock_set;
    is_caps_lock_set_ = os_events[i].is_caps_lock_set;
    mouse_x_ = os_events[i].cursor_x - window_x_;
    mouse_y_ = os_events[i].cursor_y - window_y_;

    // Update the total mouse motion - note we do not actually send events until the time exceeds
    // kTimeGranularity, at which point we send them in the above loop.
    if (os_events[i].key == kNoKey) {
      mouse_dx_ += os_events[i].mouse_dx;
      mouse_dy_ += os_events[i].mouse_dy;
      continue;
    }

    // Process all key up/key down os_events from this phase
    ASSERT(!os_events[i].key.IsDerivedKey());
    SetNonDerivedKeyPressAmount(os_events[i].key, os_events[i].press_amount);
  }

  // Fill the down keys vector
  for (int i = kMinDevice; i <= GetMaxDevice(); i++)
  for (int j = 0; j < GetNumKeys(i); j++)
  if (GetKeyState(GlopKey(j, i))->IsDownFrame())
    down_keys_frame_.push_back(GlopKey(j, i));
}

void Input::OnKeyEvent(const KeyEvent &event) {
  if (event.IsPress())
  for (int i = 0; i < (int)event.keys.size(); i++)
    pressed_keys_frame_.push_back(event.keys[i]);
  window_->OnKeyEvent(event);
  for (List<KeyListener*>::iterator it = key_listeners_.begin();
       it != key_listeners_.end(); ++it)
    (*it)->OnKeyEvent(event);
}

// Updates the press amount of a non-derived key, propogating the effect to derived keys, and
// handling any events caused in the meantime.
void Input::SetNonDerivedKeyPressAmount(const GlopKey &key, float press_amount) {
  // Get the key information and adjust the press amount to account for deadzone
  KeyTracker *info = GetNonDerivedKeyTracker(key);
  if (key.IsJoystickAxis())
    press_amount = (press_amount - kJoystickAxisThreshold) / (1 - kJoystickAxisThreshold);
  float old_press_amount = info->GetPressAmountNow();
  press_amount = max(press_amount, 0.0f);
  KeyEvent::Type type = info->SetPressAmount(press_amount);
  vector<GlopKey> keys_affected;

  // Handle events
  if (type == KeyEvent::Nothing && press_amount == old_press_amount)
    return;
  UpdateDerivedKeyStatesAndProcessEvents(key, type);
}

// Recalculates all state for derived keys that could depend on key, and generates events for them.
// Also, handle all events explicitly given to us in events_so_far
void Input::UpdateDerivedKeyStatesAndProcessEvents(const GlopKey &key, KeyEvent::Type event_type) {
  // Update derived key states
  vector<GlopKey> released_keys;
  if (event_type == KeyEvent::Release)
    released_keys.push_back(key);
  if (key.IsJoystickKey())
    UpdateDerivedKeyState(GlopKey(key.index, kDeviceAnyJoystick), &released_keys);
  for (int k = 0; k < GetNumDerivedKeys(); k++)
    UpdateDerivedKeyState(GlopKey(k, kDeviceDerived), &released_keys);

  // Handle releases
  if (released_keys.size() > 0)
    OnKeyEvent(KeyEvent(released_keys, KeyEvent::Release));

  // Handle presses
  if (event_type != KeyEvent::Nothing && event_type != KeyEvent::Release) {
    vector<GlopKey> pressed_keys(1, key);
    for (int pass = 0; pass < 2; pass++)
    for (int i = 0; i < (pass == 0? kNumJoystickKeys : GetNumDerivedKeys()); i++) {
      GlopKey derived_key(i, (pass == 0? kDeviceAnyJoystick : kDeviceDerived));
      bool is_active = false;
      for (int j = 0; j < (int)pressed_keys.size(); j++)
        is_active |= IsDerivedKeyBindingActive(derived_key, pressed_keys[j]);
      if (is_active) {
        pressed_keys.push_back(derived_key);
        GetKeyState(derived_key)->OnKeyEvent(event_type);
      }
    }
    OnKeyEvent(KeyEvent(pressed_keys, event_type));
  }
}

// Recalculates all state for a derived key, and appends any events caused by this into the
// key_events vector.
void Input::UpdateDerivedKeyState(const GlopKey &key, vector<GlopKey> *released_keys) {
  float amount = 0.0f;
  bool is_down = false;
 
  // Get press amount for any-joystick derived keys
  if (key.device == kDeviceAnyJoystick) {
    for (int i = 0; i < GetNumJoysticks(); i++)
      amount += GetKeyPressAmountNow(GlopKey(key.index, i));
    if (amount > 0)
      is_down = true;
  }

  // Get press amount for normal derived keys
  else {
    for (int i = 0; i < (int)derived_key_bindings_[key.index].size(); i++) {
      bool is_binding_down = true;
      for (int j = 0; j < (int)derived_key_bindings_[key.index][i].modifiers.size(); j++)
      if (IsKeyDownNow(derived_key_bindings_[key.index][i].modifiers[j]) !=
          derived_key_bindings_[key.index][i].down[j])
        is_binding_down = false;
      if (is_binding_down && IsKeyDownNow(derived_key_bindings_[key.index][i].key)) {
        is_down = true;
        amount += GetKeyPressAmountNow(derived_key_bindings_[key.index][i].key);
      }
    }
  }

  // Update the state and return value
  KeyState *state = GetKeyState(key);
  state->SetPressAmount(amount);
  if (state->IsDownNow() != is_down) {
    state->SetIsDown(is_down, false);
    if (!is_down)
      released_keys->push_back(key);
  }
}

// Returns whether derived_key is down and it has an active binding with query_key as the primary
// key in that binding. Technically:
//  - Returns true if derived_key is device kDeviceAnyJoystick and query_key is the corresponding
//    key on a joystick.
//  - Returns true if derived_key is device kDeviceDerived and query_key is the main key in a
//    binding all of whose modifiers are in the correct position.
bool Input::IsDerivedKeyBindingActive(const GlopKey &derived_key, const GlopKey &query_key) {
  // Make sure the key is pressed first of all
  if (!IsKeyDownNow(derived_key))
    return false;

  // Handle joystick derived keys.
  if (derived_key.device == kDeviceAnyJoystick) {
    return derived_key.index == query_key.index && query_key.IsJoystickKey();
  } 
  
  // Handle standard derived keys
  else {
    for (int i = 0; i < (int)derived_key_bindings_[derived_key.index].size(); i++)
    if (derived_key_bindings_[derived_key.index][i].key == query_key) {
      bool is_binding_down = true;
      for (int j = 0; j < (int)derived_key_bindings_[derived_key.index][i].modifiers.size(); j++)
      if (IsKeyDownNow(derived_key_bindings_[derived_key.index][i].modifiers[j]) !=
          derived_key_bindings_[derived_key.index][i].down[j])
        is_binding_down = false;
      if (is_binding_down)
        return true;
    }
  }
  return false;
}


// Returns a pointer to the KeyState corresponding to a given GlopKey.
const Input::KeyState *Input::GetKeyState(const GlopKey &key) const {
  switch (key.device) {
    case kDeviceKeyboard:
      ASSERT(key.index >= 0 && key.index < kNumKeyboardKeys);
      return &keyboard_key_trackers_[key.index].GetState();
    case kDeviceAnyJoystick:
      ASSERT(key.index >= 0 && key.index < kNumJoystickKeys);
      return &any_joystick_key_states_[key.index];
    case kDeviceDerived:
      ASSERT(key.index >= 0 && key.index < GetNumDerivedKeys());
      return &derived_key_states_[key.index];
    default:
      ASSERT(key.device >= 0 && key.index >= 0 && key.index < kNumJoystickKeys);
      if (key.device < GetNumJoysticks())
        return &joystick_key_trackers_[key.device][key.index].GetState();
      else
        return 0;
  };
}

Input::KeyTracker *Input::GetNonDerivedKeyTracker(const GlopKey &key) {
  switch (key.device) {
    case kDeviceKeyboard:
      return &keyboard_key_trackers_[key.index];
    default:
      return &joystick_key_trackers_[key.device][key.index];
  };
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
