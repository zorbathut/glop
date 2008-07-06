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
    press_amount_virtual_ = amount;
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
  else if (is_down_now_ && release_delay_ == 0) {
    press_amount_virtual_ = 0;
    is_down_now_ = false;
    was_released_ = true;
    return KeyEvent::Release;
  } else if (is_down_now_ && !keep_press_amount_on_release_delay_) {
    press_amount_virtual_ = 0;
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
  press_amount_frame_ = (press_amount_frame_ * total_frame_time_ + press_amount_virtual_ * dt) /
                        (total_frame_time_ + dt);
  total_frame_time_ += dt;

  if (is_down_now_) {
    // Handle releases
    if (press_amount_now_ == 0) {
      release_delay_left_ -= dt;
      if (release_delay_left_ <= 0) {
        press_amount_virtual_ = 0;
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
    BindDerivedKey(kGuiKeyPageDown, kKeyPageDown);
    BindDerivedKey(kGuiKeyUp, kKeyUp);
    BindDerivedKey(kGuiKeyRight, kKeyRight);
    BindDerivedKey(kGuiKeyDown, kKeyDown);
    BindDerivedKey(kGuiKeyLeft, kKeyLeft);
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

void Input::BindDerivedKey(const GlopKey &derived_key, const GlopKey &key, const GlopKey &modifier1,
                           const GlopKey &modifier2, bool down1, bool down2) {
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
  GetKeyTracker(kMouseUp)->SetReleaseDelay(100, false);
  GetKeyTracker(kMouseRight)->SetReleaseDelay(100, false);
  GetKeyTracker(kMouseDown)->SetReleaseDelay(100, false);
  GetKeyTracker(kMouseLeft)->SetReleaseDelay(100, false);
  GetKeyTracker(kMouseWheelUp)->SetReleaseDelay(150, true);
  GetKeyTracker(kMouseWheelDown)->SetReleaseDelay(150, true);
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

  // What has happened since our last poll? Even if we have gone out of focus, we should still
  // check the polling thread so that the information gets cleared.
  vector<Os::KeyEvent> os_events = Os::GetInputEvents(window_->os_data_);
  Os::GetWindowPosition(window_->os_data_, &window_x_, &window_y_);
  int n = (int)os_events.size();
  ASSERT(n > 0);

  // If we have lost focus, clear all key state. Note that down_keys_frame_ is rebuilt every frame
  // regardless, so we do not need to worry about it here.
  if (lost_focus) {
    is_num_lock_set_ = os_events[n - 1].is_num_lock_set;
    is_caps_lock_set_ = os_events[n - 1].is_caps_lock_set;
    mouse_x_ = os_events[n - 1].cursor_x - window_x_;
    mouse_y_ = os_events[n - 1].cursor_y - window_y_;
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
  // key os_events might have been generated before losing focus, and the corresponding up key event
  // may never happen.
  if (!lost_focus)
  for (int i = 0; i < n; i++) {
    // Calculate the time differential for this phase and add mouse motion os_events. Note that we
    // do not assume the times are strictly increasing. This is because the dummy input event is
    // likely generated asynchronously from the other os_events, and could thus cause an inconsistent
    // ordering.
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
      for (int k = 0; k < GetNumKeys(j); k++) {
        KeyTracker *info = GetKeyTracker(GlopKey(k, j));
        KeyEvent::Type type = info->OnKeyEventDt(kTimeGranularity);
        if (type != KeyEvent::Nothing)
          OnKeyEvent(KeyEvent(GlopKey(k, j), type), 0);
      }
      float mouse_scale = mouse_sensitivity_ * kBaseMouseSensitivity / kTimeGranularity;
      OnOsKeyEvent(kMouseUp, -mouse_dy_ * mouse_scale);
      OnOsKeyEvent(kMouseRight, mouse_dx_ * mouse_scale);
      OnOsKeyEvent(kMouseDown, mouse_dy_ * mouse_scale);
      OnOsKeyEvent(kMouseLeft, -mouse_dx_ * mouse_scale);
      mouse_dx_ = mouse_dy_ = 0;
      OnKeyEvent(KeyEvent(kNoKey, KeyEvent::Nothing), kTimeGranularity);
    }
    if (i == n) break;

    // Update the new settings
    is_num_lock_set_ = os_events[i].is_num_lock_set;
    is_caps_lock_set_ = os_events[i].is_caps_lock_set;
    mouse_x_ = os_events[i].cursor_x - window_x_;
    mouse_y_ = os_events[i].cursor_y - window_y_;

    // Add mouse mouse os_events - note this requires dt > 0.
    if (os_events[i].key == kNoKey) {
      mouse_dx_ += os_events[i].mouse_dx;
      mouse_dy_ += os_events[i].mouse_dy;
      continue;
    }

    // Process all key up/key down os_events from this phase
    ASSERT(!os_events[i].key.IsDerivedKey());
    OnOsKeyEvent(os_events[i].key, os_events[i].press_amount);
  }

  // Fill the down keys vector
  for (int i = kMinDevice; i <= GetMaxDevice(); i++)
  for (int j = 0; j < GetNumKeys(i); j++)
  if (GetKeyTracker(GlopKey(j, i))->IsDownFrame())
    down_keys_frame_.push_back(GlopKey(j, i));
}

// Handles a message from the operating system that a non-derived key is pressed the given amount.
// We generate regular input events if appropriate, and also update the derived keys.
void Input::OnOsKeyEvent(const GlopKey &key, float press_amount) {
  // Get the key information and press amount (accounting for the deadzone)
  KeyTracker *info = GetKeyTracker(key);
  if (key.IsJoystickKey())
    press_amount = (press_amount - kJoystickAxisThreshold) / (1 - kJoystickAxisThreshold);
  press_amount = max(press_amount, 0.0f);
  if (press_amount == info->GetPressAmountNow())
    return;
  KeyEvent::Type type = info->SetPressAmount(press_amount);
  if (type != KeyEvent::Nothing)
    OnKeyEvent(KeyEvent(key, type), 0);

  // Update derived keys that could possibly be affected
  if (key.IsJoystickKey()) {
    if (key.IsJoystickAxis()) {
      UpdateDerivedKey(GetJoystickAxisPos(key.GetJoystickAxisNumber()));
      UpdateDerivedKey(GetJoystickAxisNeg(key.GetJoystickAxisNumber()));
    } else {
      UpdateDerivedKey(GlopKey(key.index, kDeviceAnyJoystick));
    }
  }
  for (int k = 0; k < GetNumDerivedKeys(); k++)
    UpdateDerivedKey(GlopKey(k, kDeviceDerived));
}

// Recalculates how pressed a derived key is, and then updates our internal records, possibly
// sending out event notifications.
void Input::UpdateDerivedKey(const GlopKey &key) {
  float amount = 0.0f;

  // Get press amount for normal derived keys
  if (key.device == kDeviceDerived) {
    for (int i = 0; i < (int)derived_key_bindings_[key.index].size(); i++) {
      bool is_binding_down = true;
      for (int j = 0; j < (int)derived_key_bindings_[key.index][i].modifiers.size(); j++)
      if (IsKeyDownNow(derived_key_bindings_[key.index][i].modifiers[j]) !=
          derived_key_bindings_[key.index][i].down[j])
        is_binding_down = false;
      if (is_binding_down)
        amount = max(amount, GetKeyPressAmountNow(derived_key_bindings_[key.index][i].key));
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
  for (List<KeyListener*>::iterator it = key_listeners_.begin(); it != key_listeners_.end(); ++it)
    (*it)->OnKeyEvent(event, dt);
}

// Returns a pointer to the KeyTracker corresponding to a given GlopKey.
const Input::KeyTracker *Input::GetKeyTracker(const GlopKey &key) const {
  switch (key.device) {
    case kDeviceKeyboard:
      ASSERT(key.index >= 0 && key.index < kNumKeyboardKeys);
      return &keyboard_key_trackers_[key.index];
    case kDeviceAnyJoystick:
      ASSERT(key.index >= 0 && key.index < kNumJoystickKeys);
      return &any_joystick_key_trackers_[key.index];
    case kDeviceDerived:
      ASSERT(key.index >= 0 && key.index < GetNumDerivedKeys());
      return &derived_key_trackers_[key.index];
    default:
      ASSERT(key.device >= 0 && key.index >= 0 && key.index < kNumJoystickKeys);
      if (key.device < GetNumJoysticks())
        return &joystick_key_trackers_[key.device][key.index];
      else
        return 0;
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
