// The interface for all keyboard, mouse, and joystick I/O under Glop using a unified GlopKey
// mechanism that includes everything from keyboard keys to joystick axes to mouse motion.
//
// Input can be tracked in two very different ways, depending on whether you want simplicity or
// more fine-grained control. The simplest mechanism is to query the Input class after a call
// to system()->Think. This is simple and usually effective. However, this approach does not allow
// you to gain any context information about the input state when a key was pressed. For example,
// the following tasks are impossible:
//   - Determining the order in which various keys were pressed.
//   - Determining what keys were down, or where the mouse was, or where a joystick axis was, at
//     the time in which a key was pressed.
//   - Determining how often or at what time a key was pressed during a single frame.
//
// If it is necessary to know the answers to these questions, as is often the case for GUI
// controls, a KeyListener must be used. (Note that a GlopFrame can use KeyListeners implicitly via
// its OnKeyDown method.) A KeyListener receives notification on any key "event", annotated with the
// time elapsed since the last event:
//   - Nothing: These events are sent periodically (many times a second) whenever Input has
//     potentially changed state. This is the only event where dt > 0.
//   - KeyPress: A key has gone from up to down. See also KeyDoublePress.
//   - KeyDoublePress: A key has been pressed twice rapidly in succession. Also generates a KeyPress
//                     for the first press.
//   - KeyRepeat: A key has been held down for a long period of time, and a pseudo-key press is
//                registered. Similar to text editors that detect key "presses" while a key is held
//                down.
//   - KeyRelease: A key has gone from down to up.
// The KeyListener can then perform any logic it desires, and during this time, the Input class can
// be queried for the input state as of the keypress. Note: this mechanism is still potentially
// imperfect. The Glop Os implementation is only required to synch up keyboard presses with each
// other. All other keys could be misordered with an error of about 5 ms.
//
// Once it comes times to actually query the Input, several options are available:
//  GetMouseX, IsNumLockSet, etc.: Input state independent of the individual GlopKeys.
//  GetAsciiValue: Converts a key to ASCII. This should be used in a KeyListener. Otherwise, the
//                 shift, alt, control, num lock & caps lock state may be out of date.
//  GetKeyPressAmount: Returns the degree to which a key is down (usually between 0 and 1). Fully
//                     supports analog controls. Suitable for moving an object on-screen.
//                       Example usage: player_x += GetKeyPressAmountFrame(kPlayerRightKey) * dt;
//                     Either the current press amount or an average press amount over the frame
//                     can be queried.
//  IsKeyDown: This is a discrete version of GetKeyPressAmount, and is usually equivalent to
//             querying whether GetKeyPressAmount > 0. For certain keys, however, Input may
//             artificially declare a key to be down shortly after it is released to promote
//             smoothness (e.g. for mouse motion). 
//               Example usage: player_shield_on = IsKeyDownFrame(kShieldKey);
//             Either the current state or the whole frame can be queried. In the latter case, true
//             is returned if the key was ever down during the frame.
//  WasKeyPressed, WasKeyReleased: Convenience functions that return whether a given key event
//                                 was generated this frame.
//
// Finally, it is not always convenient to work directly with one input key at a time. For example,
// it is sometimes useful to consider two keys linked together as one - e.g. kKeyEitherShift is the
// same as kKeyLeftShift || kKeyRightShift. These are called derived keys. Manually checking if a
// derived key is down would be easy, but generating events for them is a little more complicated.
// To facilitate this, DerivedKeys may be defined directly wihin Input and then events will be
// generated for them as if they were normal keys. Alternatively, Input::KeyTracker can be used to
// simulate events for a a user-controlled "key". This is used, for example, by Gui buttons.
//
// Note that the Input class silently uses a separate thread for polling the Os input. This ensures
// that the input will be accurate even if the frame rate is bad.

#ifndef GLOP_INPUT_H__
#define GLOP_INPUT_H__

// Includes
#include "Base.h"
#include "List.h"
#include <string>
#include <vector>
using namespace std;

// Class declarations
class Input;
struct OsKeyEvent;
struct OsWindowData;

// Key repeat-rate constants
const int kRepeatDelay = 500;  // Time (in ms) between key down event #1 and #2 while a key is down
const int kRepeatRate = 60;    // Time (in ms) between later key down events while a key is down

// GlopKey devices
const int kDeviceKeyboard = -1;
const int kDeviceAnyJoystick = -2;
const int kDeviceDerived = -3;
const int kMinDevice = -3;

// GlopKey
// =======

// This is a basic identifier for any kind of key.
// To create a GlopKey, use the static methods and constants below.
struct GlopKey {
  // Constructor
  GlopKey(int _index = -2, int _device = kDeviceKeyboard): device(_device), index(_index) {}
  short index, device;

  // Comparators
  bool operator==(const GlopKey &rhs) const {
    return device == rhs.device && index == rhs.index;
  }
  bool operator!=(const GlopKey &rhs) const {
    return device != rhs.device || index != rhs.index;
  }
  bool operator<(const GlopKey &rhs) const {
    return (device == rhs.device? device < rhs.device : index < rhs.index);
  }

  // Key property queries. Most are self-explanatory, but:
  //  GetName: Returns a string description of the key. For example, "Enter" or
  //           "Joystick #1 - Button #4".
  //  IsTrackable: All "keys" generate key press events when they are first pressed. Our other
  //               functionality (IsKeyDown, GetKeyPressAmount, and repeat events) requires being
  //               able to track the key's state. This returns whether that is possible on all OSes.
  //  IsDerivedKey: Certain GlopKeys do not correspond to a single key - for example,
  //                kKeyEitherShift or any key with kAnyJoystick. This returns whether that applies
  //                to this key.
  //  IsMouseMotion: Includes mouse wheel.
  //  IsMotionKey: Does this key correspond to something that a user likely would consider more
  //               like scrolling or moving a pointer, rather than an actual key? Currently, this
  //               is mouse motion and joystick axes.
  //  IsModifierKey: Is this a non-motion key that a user would probably still not consider to
  //                 be a "real" key. Currently: alt, shift, and control.
  //  GetJoystickAxisNum, etc.: Assuming this is a joystick axis key of some kind, which axis is
  //                            it tied to?
  const string GetName() const;
  bool IsTrackable() const;
  bool IsDerivedKey() const;
  bool IsKeyboardKey() const;
  bool IsMouseKey() const;
  bool IsJoystickKey() const;
  bool IsMouseMotion() const;
  bool IsJoystickAxis() const;
  bool IsJoystickAxisPos() const;
  bool IsJoystickAxisNeg() const;
  bool IsJoystickHat() const;
  bool IsJoystickHatUp() const;
  bool IsJoystickHatRight() const;
  bool IsJoystickHatDown() const;
  bool IsJoystickHatLeft() const;
  bool IsJoystickButton() const;
  bool IsMotionKey() const;
  bool IsModifierKey() const;
  int GetJoystickAxisNumber() const;
  int GetJoystickHatNumber() const;
  int GetJoystickButtonNumber() const;
};

// Key constants - miscellaneous
const GlopKey kNoKey(-2);
const GlopKey kAnyKey(-1);

// Key constants - keyboard. General rules:
//  - If a key has a clear ASCII value associated with it, its GlopKey is given just by that ASCII
//    value. For example, Enter is 13.
//  - The non-shift, non-caps lock ASCII value is used when there's ambiguity. Thus, 'a' and ','
//    are valid GlopKeys, but 'A' and '<' are not valid GlopKeys.
//  - Key-pad keys are NEVER given GlopKeys based on their ASCII values.
// Note: these ASCII-related key indices are not intended to replace ASCII. If you want an ASCII
// value, call Input::ToAscii so that shift, num lock and caps lock are all considered.
const GlopKey kKeyBackspace(8);
const GlopKey kKeyTab(9);
const GlopKey kKeyEnter(13);
const GlopKey kKeyReturn(13);
const GlopKey kKeyEscape(27);

const GlopKey kKeyF1(129);
const GlopKey kKeyF2(130);
const GlopKey kKeyF3(131);
const GlopKey kKeyF4(132);
const GlopKey kKeyF5(133);
const GlopKey kKeyF6(134);
const GlopKey kKeyF7(135);
const GlopKey kKeyF8(136);
const GlopKey kKeyF9(137);
const GlopKey kKeyF10(138);
const GlopKey kKeyF11(139);
const GlopKey kKeyF12(140);

const GlopKey kKeyCapsLock(150);
const GlopKey kKeyNumLock(151);
const GlopKey kKeyScrollLock(152);
const GlopKey kKeyPrintScreen(153);
const GlopKey kKeyPause(154);
const GlopKey kKeyLeftShift(155);
const GlopKey kKeyRightShift(156);
const GlopKey kKeyLeftControl(157);
const GlopKey kKeyRightControl(158);
const GlopKey kKeyLeftAlt(159);
const GlopKey kKeyRightAlt(160);

const GlopKey kKeyRight(166);
const GlopKey kKeyLeft(167);
const GlopKey kKeyUp(168);
const GlopKey kKeyDown(169);

const GlopKey kKeyPadDivide(170);
const GlopKey kKeyPadMultiply(171);
const GlopKey kKeyPadSubtract(172);
const GlopKey kKeyPadAdd(173);
const GlopKey kKeyPadEnter(174);
const GlopKey kKeyPadDecimal(175);
const GlopKey kKeyPadEquals(176);
const GlopKey kKeyPad0(177);
const GlopKey kKeyPad1(178);
const GlopKey kKeyPad2(179);
const GlopKey kKeyPad3(180);
const GlopKey kKeyPad4(181);
const GlopKey kKeyPad5(182);
const GlopKey kKeyPad6(183);
const GlopKey kKeyPad7(184);
const GlopKey kKeyPad8(185);
const GlopKey kKeyPad9(186);

const GlopKey kKeyDelete(190);
const GlopKey kKeyHome(191);
const GlopKey kKeyInsert(192);
const GlopKey kKeyEnd(193);
const GlopKey kKeyPageUp(194);
const GlopKey kKeyPageDown(195);

// Key constants - mouse
const GlopKey kMouseUp(291);
const GlopKey kMouseRight(292);
const GlopKey kMouseDown(293);
const GlopKey kMouseLeft(294);
const GlopKey kMouseWheelUp(295);
const GlopKey kMouseWheelDown(296);
const GlopKey kMouseLButton(297);
const GlopKey kMouseMButton(298);
const GlopKey kMouseRButton(299);
const int kFirstMouseKeyIndex = 291;
const int kNumKeyboardKeys = 300;

// Key constants - joystick
const int kJoystickAxisStart = 0;
const int kJoystickAxisPos = kJoystickAxisStart + 0;
const int kJoystickAxisNeg = kJoystickAxisStart + 1;
const int kJoystickAxisRight = kJoystickAxisStart + 0;
const int kJoystickAxisLeft = kJoystickAxisStart + 1;
const int kJoystickAxisUp = kJoystickAxisStart + 2;
const int kJoystickAxisDown = kJoystickAxisStart + 3;
const int kJoystickNumAxes = 6;
const int kJoystickAxisEnd = kJoystickAxisStart + 2 * kJoystickNumAxes;
const int kJoystickHatStart = kJoystickAxisEnd;
const int kJoystickHatUp = kJoystickHatStart + 0;
const int kJoystickHatRight = kJoystickHatStart + 1;
const int kJoystickHatDown = kJoystickHatStart + 2;
const int kJoystickHatLeft = kJoystickHatStart + 3;
const int kJoystickNumHats = 4;
const int kJoystickHatEnd = kJoystickHatStart + 4 * kJoystickHatStart;
const int kJoystickButtonStart = kJoystickHatEnd;
const int kJoystickNumButtons = 32;
const int kJoystickButtonEnd = kJoystickButtonStart + kJoystickNumButtons;
const int kNumJoystickKeys = kJoystickButtonEnd;

// Joystick key creators
inline GlopKey GetJoystickUp(int joystick = kDeviceAnyJoystick) {
  return GlopKey(kJoystickAxisUp, joystick);
}
inline GlopKey GetJoystickRight(int joystick = kDeviceAnyJoystick) {
  return GlopKey(kJoystickAxisRight, joystick);
}
inline GlopKey GetJoystickDown(int joystick = kDeviceAnyJoystick) {
  return GlopKey(kJoystickAxisDown, joystick);
}
inline GlopKey GetJoystickLeft(int joystick = kDeviceAnyJoystick) {
  return GlopKey(kJoystickAxisLeft, joystick);
}
inline GlopKey GetJoystickAxisPos(int axis, int joystick = kDeviceAnyJoystick) {
  return GlopKey(kJoystickAxisPos + 2*axis, joystick);
}
inline GlopKey GetJoystickAxisNeg(int axis, int joystick = kDeviceAnyJoystick) {
  return GlopKey(kJoystickAxisNeg + 2*axis, joystick);
}
inline GlopKey GetJoystickHatUp(int hat, int joystick = kDeviceAnyJoystick) {
  return GlopKey(kJoystickHatUp + hat*4, joystick);
}
inline GlopKey GetJoystickHatRight(int hat, int joystick = kDeviceAnyJoystick) {
  return GlopKey(kJoystickHatRight + hat*4, joystick);
}
inline GlopKey GetJoystickHatDown(int hat, int joystick = kDeviceAnyJoystick) {
  return GlopKey(kJoystickHatDown + hat*4, joystick);
}
inline GlopKey GetJoystickHatLeft(int hat, int joystick = kDeviceAnyJoystick) {
  return GlopKey(kJoystickHatLeft + hat*4, joystick);
}
inline GlopKey GetJoystickButton(int button, int joystick = kDeviceAnyJoystick) {
  return GlopKey(kJoystickButtonStart + button, joystick);
}

// Non-editable derived keys
const GlopKey kKeyEitherShift(0, kDeviceDerived);
const GlopKey kKeyEitherControl(1, kDeviceDerived);
const GlopKey kKeyEitherAlt(2, kDeviceDerived);
const int kNumFixedDerivedKeys = 3;

// User-editable derived keys
const GlopKey kGuiKeyPageUp(kNumFixedDerivedKeys + 0, kDeviceDerived);
const GlopKey kGuiKeyPageRight(kNumFixedDerivedKeys + 1, kDeviceDerived);
const GlopKey kGuiKeyPageDown(kNumFixedDerivedKeys + 2, kDeviceDerived);
const GlopKey kGuiKeyPageLeft(kNumFixedDerivedKeys + 3, kDeviceDerived);
const GlopKey kGuiKeyScrollUp(kNumFixedDerivedKeys + 4, kDeviceDerived);
const GlopKey kGuiKeyScrollRight(kNumFixedDerivedKeys + 5, kDeviceDerived);
const GlopKey kGuiKeyScrollDown(kNumFixedDerivedKeys + 6, kDeviceDerived);
const GlopKey kGuiKeyScrollLeft(kNumFixedDerivedKeys + 7, kDeviceDerived);
const GlopKey kGuiKeyUp(kNumFixedDerivedKeys + 8, kDeviceDerived);
const GlopKey kGuiKeyRight(kNumFixedDerivedKeys + 9, kDeviceDerived);
const GlopKey kGuiKeyDown(kNumFixedDerivedKeys + 10, kDeviceDerived);
const GlopKey kGuiKeyLeft(kNumFixedDerivedKeys + 11, kDeviceDerived);
const GlopKey kGuiKeyConfirm(kNumFixedDerivedKeys + 12, kDeviceDerived);
const GlopKey kGuiKeyCancel(kNumFixedDerivedKeys + 13, kDeviceDerived);
const GlopKey kGuiKeyPrimaryClick(kNumFixedDerivedKeys + 14, kDeviceDerived);
const GlopKey kGuiKeySecondaryClick(kNumFixedDerivedKeys + 15, kDeviceDerived);
const GlopKey kGuiKeySelectNext(kNumFixedDerivedKeys + 16, kDeviceDerived);
const GlopKey kGuiKeySelectPrev(kNumFixedDerivedKeys + 17, kDeviceDerived);
const int kNumBasicDerivedKeys = kNumFixedDerivedKeys + 18;

// KeyEvent
// ========
//
// See comment at the top of the file.
struct KeyEvent {
  enum Type {Nothing, Release, Press, RepeatPress, DoublePress};
  KeyEvent(const GlopKey &_key, Type type): key(_key), type_(type) {}

  GlopKey key;
  bool IsNothing() const {return type_ == Nothing;}
  bool IsRelease() const {return type_ == Release;}
  bool IsPress() const {return type_ != Release && type_ != Nothing;}
  bool IsNonRepeatPress() const {return type_ == Press || type_ == DoublePress;}
  bool IsRepeatPress() const {return type_ == RepeatPress;}
  bool IsDoublePress() const {return type_ == DoublePress;}

 private:
  Type type_;
};

// KeyListener
// ===========
//
// See comment at the top of the file. Allows a class to receive key events.
class KeyListener {
 public:
  KeyListener(bool begin_listening = true): listener_id_(0) {
    if (begin_listening) BeginKeyListening();
  }
  ~KeyListener() {StopKeyListening();}

 protected:
  // Overload this to take action on a KeyEvent.
  virtual void OnKeyEvent(const KeyEvent &event, int dt) = 0;

  // Utility functions that start and stop KeyEvent messages from being sent here. These are called
  // automatically on creation and deletion, but are provided as a convenience at all times.
  void BeginKeyListening();
  void StopKeyListening();
 private:
  ListId listener_id_;
  friend class Input;
  DISALLOW_EVIL_CONSTRUCTORS(KeyListener);
};

// Input
// =====

// Static accessor convenience method
Input *input();

class Input {
 public:
  int GetMaxDevice() const {return GetNumJoysticks() - 1;}

  // KeyTracker
  // ==========
  //
  // A KeyTracker simulates the logic that generates key events based on whether a given key is up
  // or down (e.g. repeat events and double-clicks). This is used internally by Input, but it is
  // also provided publicly.
  class KeyTracker {
  public:
    // Creates a new key - release_delay is an optional artifical delay that ensures the key is
    // registered as down for a little while even after it is marked as up. This is useful for very
    // choppy "keys", such as mouse motion.
    KeyTracker(int release_delay = 0);
    void SetReleaseDelay(int delay, bool keep_press_amount) {
      release_delay_ = delay;
      keep_press_amount_on_release_delay_ = keep_press_amount;
    }

    // Marks the key as up, ignoring the release_delay. Useful, for example, when focus is lost.
    // Returns any event caused by this: None or Release.
    KeyEvent::Type Clear();
    
    // Sets the current key press amount.
    // Returns any event caused by this: None, Press, Release (if release_delay == 0) or DoublePress.
    KeyEvent::Type SetPressAmount(float amount);
    KeyEvent::Type SetIsDown(bool is_down) {return SetPressAmount(is_down? 1.0f : 0.0f);}

    // Registers that a certain time has passed.
    // Returns any event caused by this: None, RepeatPress, or Release (if release_delay > 0).
    KeyEvent::Type OnKeyEventDt(int dt);

    // Registers the start of a new frame.
    void Think();

    // Instantaneous status - see top of the file and Input class below.
    float GetPressAmountNow() const {return press_amount_virtual_;}
    bool IsDownNow() const {return is_down_now_;}

    // Frame status - see top of the file and Input class below.
    float GetPressAmountFrame() const {return press_amount_frame_;}
    bool IsDownFrame() const {return is_down_frame_;}
    bool WasPressed(bool count_repeats = true) const {
      return count_repeats? was_pressed_ : was_pressed_no_repeats_;
    }
    bool WasReleased() const {return was_released_;}
    
  private:
    float press_amount_now_,      // Instantaneous press_amount for the key
          press_amount_frame_,    // Total press_amount for the key this frame
          press_amount_virtual_;  // What we claim our press amount is
    bool is_down_now_,            // Is the key down now - not quite the same as cur_press_amount>0
         is_down_frame_;          // Was the key down at any point this frame (See IsKeyDown)
    int total_frame_time_;        // Total time spent during this frame
    int release_delay_left_,      // Minimum time until cur_is_down can be reverted to false
        release_delay_;           // The max value for release_delay_left_
    bool keep_press_amount_on_release_delay_; // If we artifically keeping the pressed down, do we
                                              // hold it's press amount, or claim it is 0?       
    int double_press_time_left_;  // Used for tracking double presses
    int repeat_delay_left_;       // Time remaining until we generate a repeat event
    bool was_pressed_,            // Was a key press event generated for this key this frame
         was_pressed_no_repeats_, // Was a key press (no repeat) event generated this frame
         was_released_;           // Was a key release event generated for this key this frame
  };

  // Gets/sets a sensitivity which is used to scale kMouseLeft, kMouseTop, etc. every frame. This
  // has no effect on mouse cursor position.
  float GetMouseSensitivity() const {return mouse_sensitivity_;}
  void SetMouseSensitivity(float sensitivity) {mouse_sensitivity_ = sensitivity;}

  // Returns the number of joysticks attached. This (and corresponding key events) will only be
  // updated to reflect hardware changes if RefreshJoysticks is called. RefreshJoysticks may be
  // safely called as often as once a frame, and it will ensure an update happens soon (but not
  // always immediately).
  int GetNumJoysticks() const {return num_joysticks_;}
  void RefreshJoysticks() {requested_joystick_refresh_ = true;}
  
  // Instantaneous input status
  // ==========================
  //
  // These queries are best performed inside a KeyListener where the information will be fully up
  // to date.
  
  // Note that ASCII value depends on both the key, and the state of the rest of the keyboard
  // (e.g. shift, caps lock, and num lock).
  unsigned char GetAsciiValue(const GlopKey &key) const;

  // Returns whether num lock and caps lock are currently enabled
  bool IsNumLockSet() const {return is_num_lock_set_;}
  bool IsCapsLockSet() const {return is_caps_lock_set_;}

  // Queries whether a particular key is down, or how far down it is. See comment at the top of
  // the file.
  float GetKeyPressAmountNow(const GlopKey &key) const {
    const KeyTracker *info = GetKeyTracker(key);
    return (info == 0? 0.0f : info->GetPressAmountNow());
  }
  bool IsKeyDownNow(const GlopKey &key) const {
    const KeyTracker *info = GetKeyTracker(key);
    return (info == 0? false : info->IsDownNow());
  }

  // Gets/sets mouse position. All coordinates are relative to the top-left corner of the window
  // owning the Input class. Note that GetMouseX and GetMouseY are provided mainly for GUI support.
  // Features such as mouse look are implemented better using key tracking with kMouseUp, etc.
  void SetMousePosition(int x, int y);
  int GetMouseX() const {return mouse_x_;}
  int GetMouseY() const {return mouse_y_;}

  // Shows or hides the mouse cursor. Note that even if IsMouseCursorShown() == false, the cursor
  // will still be visible outside the window or if the window is not in focus.
  void ShowMouseCursor(bool is_visible);
  bool IsMouseCursorShown() const {return is_cursor_visible_;}

  // Frame key status
  // ================
  //
  // Queries whether a particular key was down this frame, or how far down it was. See comment at
  // the top of the file.
  float GetKeyPressAmountFrame(const GlopKey &key) const {
    const KeyTracker *info = GetKeyTracker(key);
    return (info == 0? 0.0f : info->GetPressAmountFrame());
  }
  bool IsKeyDownFrame(const GlopKey &key) const {
    const KeyTracker *info = GetKeyTracker(key);
    return (info == 0? false : info->IsDownFrame());
  }
  const vector<GlopKey> &GetDownKeysFrame() const {return down_keys_frame_;}

  // Was a key event generated for this key during this frame? WasKeyPressed counts repeat events
  // only if count_repeats == true.
  bool WasKeyPressed(const GlopKey &key, bool count_repeats = true) const {
    const KeyTracker *info = GetKeyTracker(key);
    return (info == 0? false : info->WasPressed(count_repeats));
  }
  bool WasKeyReleased(const GlopKey &key) const {
    const KeyTracker *info = GetKeyTracker(key);
    return (info == 0? false : info->WasReleased());
  }

  // Returns a single key that has been pressed this frame (including repeat events). If no key
  // has been pressed, kNoKey is returned. Derived keys will never be returned - a corresponding
  // normal key will be returned instead.
  // accept_clicks can be used to filter out mouse clicks, accept_modifiers and accept_motion
  // filter out modifier and motion keys respectively.
  const GlopKey &GetKeyPress(bool accept_clicks = true, bool accept_modifiers = false,
                             bool accept_motion = false);

  // Calls system()->think until GetKeyPress != kNoKey.
  const GlopKey &WaitForKeyPress(bool accept_clicks = true, bool accept_modifiers = false,
                                 bool accept_motion = false);

  // Derived Keys
  // ============
  static int GetNumDerivedKeys() {return (int)derived_key_names_.size();}
  static void ConfigureGuiKeys(bool keyboard_bindings, bool mouse_bindings, bool joystick_bindings);
  static GlopKey AllocateDerivedKey(const string &key_name);
  static void UnbindDerivedKey(const GlopKey &derived_key);
  static void BindDerivedKey(const GlopKey &derived_key, const GlopKey &key);
  static void BindDerivedKey(const GlopKey &derived_key, const GlopKey &key,
                             const GlopKey &modifier, bool down = true);
  static void BindDerivedKey(const GlopKey &derived_key, const GlopKey &key,
                             const GlopKey &modifier1, const GlopKey &modifier2, bool down1 = true,
                             bool down2 = true);
  static void BindDerivedKey(const GlopKey &derived_key, const GlopKey &key,
                             const vector<GlopKey> &modifiers, const vector<bool> &down);
  static void ClearDerivedKeys();

 private:
  // Interface to System
  friend class System;
  static void InitDerivedKeys();

  // Interface to GlopWindow
  friend class GlopWindow;
  Input(GlopWindow *window);
  void Think(bool lost_focus, int dt);

  // Meta-state
  GlopWindow *window_;                            // The window that owns us
  int last_poll_time_;                            // The time at which we last did a poll
  bool last_poll_time_set_;                       // Whether we have ever set last_poll_time_.
  int window_x_, window_y_;                       // Current window coordinates - used for tracking
                                                  //  mouse position. Note this is different from
                                                  //  GlopWindow coordinates, which are valid only
                                                  //  in windowed mode.
  bool is_num_lock_set_, is_caps_lock_set_;       // Are num lock and caps lock on right now?
  float mouse_sensitivity_;                       // Scale for mouse motion events
  int mouse_x_, mouse_y_;                         // The current cusors position in window coords
  int mouse_dx_, mouse_dy_;                       // Amount the mouse has moved in mickeys since we
                                                  //  last created mouse motion events.
  bool is_cursor_visible_, os_is_cursor_visible_; // Has the user set the cursor to be shown? And
                                                  //  have we told the Os to actually show it?
  int num_joysticks_;                             // The number of joysticks we have detected
  int joystick_refresh_time_;                     // The length of time since we last did a joystick
                                                  //  refresh. Used to prevent doing refreshes too
                                                  //  often.
  bool requested_joystick_refresh_;               // Whether the user wants to do a joystick refresh
  List<KeyListener*> key_listeners_;

  // Key status
  vector<GlopKey> down_keys_frame_, pressed_keys_frame_;
  KeyTracker keyboard_key_trackers_[kNumKeyboardKeys];
  vector<vector<KeyTracker> > joystick_key_trackers_;
  KeyTracker any_joystick_key_trackers_[kNumJoystickKeys];
  vector<KeyTracker> derived_key_trackers_;

  // Derived key configuration
  friend struct GlopKey;
  struct DerivedKeyBinding {
    DerivedKeyBinding(const GlopKey &_key = GlopKey()): key(_key) {}
    DerivedKeyBinding(const GlopKey &_key, const vector<GlopKey> &_modifiers,
      const vector<bool> &_down): key(_key), modifiers(_modifiers), down(_down) {}
    GlopKey key;
    vector<GlopKey> modifiers;
    vector<bool> down;
  };
  static vector<string> derived_key_names_;
  static vector<vector<DerivedKeyBinding> > derived_key_bindings_;

  // Utility functions
  void OnOsKeyEvent(const GlopKey &key, float press_amount);
  void UpdateDerivedKey(const GlopKey &key);
  void OnKeyEvent(const KeyEvent &event, int dt);
  const KeyTracker *GetKeyTracker(const GlopKey &key) const;
  KeyTracker *GetKeyTracker(const GlopKey &key) {
    const Input *const_this = (const Input *)this;
    return (KeyTracker*)const_this->GetKeyTracker(key);
  }
  void UpdateOsCursorVisibility();
  friend class KeyListener;
  DISALLOW_EVIL_CONSTRUCTORS(Input);
};

// Key iteration
inline int GetNumKeys(int device) {
  return device == kDeviceKeyboard? kNumKeyboardKeys :
         device == kDeviceDerived? input()->GetNumDerivedKeys() : kNumJoystickKeys;
}

#endif // GLOP_INPUT_H__
