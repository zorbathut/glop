// The interface for all keyboard, mouse, and joystick I/O under Glop using a unified GlopKey
// mechanism that includes everything from keyboard keys to joystick axes to mouse motion.
//
// Input can be tracked in two very different ways, depending on whether one wants simplicity or
// more fine-grained control. The simplest mechanism is to query the Input class after a call
// to gSystem->Think. This is simple and usually effective. However, this approach does not allow
// you to gain any context information about the input state when a key was pressed. For example,
// the following tasks are impossible:
//   - Determining the order in which various keys were pressed.
//   - Determining what keys were down, or where the mouse was, or where a joystick axis was, at
//     the time in which a key was pressed.
//   - Determining how often or at what time a key was pressed during a single frame.
//
// If it is necessary to know the answers to these questions, as is often the case for GUI
// controls, a KeyListener must be used. A KeyListener receives notification on any key "event",
// annotated with the time elapsed since the last event:
//   - Nothing: These events are sent periodically (many times a second) whenever Input has
//     potentially changed state. This is the only event where dt > 0.
//   - KeyPress: A key has gone from up to down. See also KeyDoublePress.
//   - KeyDoublePress: A key has been pressed twice rapidly in succession. Also generates a KeyPress
//                     for the first time (and potentially more KeyDoublePresses in the future).
//   - KeyRepeat: A key has been held down for a long period of time, and a pseudo-key press is
//                registered. Similar to text editors that detect key presses while a key is held
//                down.
//   - KeyRelease: A key has gone from down to up. See also KeyReleaseDefocus.
//   - KeyReleaseDefocus: A key has been marked as up because the GlopWindow is no longer in focus.
// The KeyListener can then perform any logic it desires, and during this time, the Input class can
// be queried for the input state as of the keypress. Note: this mechanism is still potentially
// imperfect. The Glop Os implementation is only required to synch up keyboard presses with each
// other. All other keys could be misordered with an error of about 5 ms.
//
// Once it comes times to actually query the Input, several options are available:
//  GetMouseX, IsNumLockSet, etc.: Various input state independent of the individual GlopKeys.
//  GetAsciiValue: Converts a key to ASCII. This should be used in a KeyListener. Otherwise, the
//                 shift, alt, control, num lock & caps lock state may be out of date.
//  GetKeyPressAmount: Returns the degree to which a key is down (usually between 0 and 1). Fully
//                     supports analog controls. Suitable for moving an object on-screen.
//                       Example usage: player_x += GetKeyPressAmountFrame(kPlayerRightKey) * dt;
//                     Either the current press amount or an average press amount over the frame
//                     can be queried.
//  IsKeyDown: This is a discrete version of GetKeyPressAmount, and is usually equivalent to
//             querying whether GetKeyPressAmount > 0. For certain keys, however, Input may
//             artificially declare a key down shortly after it is released to promote smoothness
//             (e.g. for mouse motion). 
//               Example usage: player_shield_on = IsKeyDownFrame(kShieldKey);
//             Either the current state or the whole frame can be queried. In the latter case, true
//             is returned if the key was ever down during the frame.
//  WasKeyPressed, WasKeyReleased: Convenience functions that return whether a given key event
//                                 was generated this frame.
//
// Note that the Input class silently uses a separate thread for polling the Os input. This ensures
// that the input will be accurate even if the frame rate is bad.

#ifndef INPUT_H__
#define INPUT_H__

// Includes
#include "Base.h"
#include "LightSet.h"
#include <string>
#include <vector>
using namespace std;

// Constants
const int kRepeatDelay = 500;  // Time (in ms) between key down event #1 and #2 while a key is down
                               //   Provided so that key behavior can be emulated.
const int kRepeatRate = 60;    // Time (in ms) between later key down events while a key is down

// Class declarations
class Input;
class InputPollingThread;
struct OsKeyEvent;
struct OsWindowData;

// GlopKey
// =======

// This is a basic identifier for any kind of key.
// To refer to a specific key, use the static methods and constants below.
struct GlopKey {
  // Constructor
  GlopKey(int _index = -2): joystick(-1), index(_index) {}
  GlopKey(int _joystick, int _index): joystick(_joystick), index(_index) {}
  short joystick, index;

  // Comparators
  bool operator==(const GlopKey &rhs) const {
    return joystick == rhs.joystick && index == rhs.index;
  }
  bool operator!=(const GlopKey &rhs) const {
    return joystick != rhs.joystick || index != rhs.index;
  }
  bool operator<(const GlopKey &rhs) const {
    return (joystick == rhs.joystick? joystick < rhs.joystick : index < rhs.index);
  }

  // Key property queries. Most are self-explanatory, but:
  //  GetName: Returns a string description of the key. For example, "Enter" or
  //           "Joystick #1 - Button #4".
  //  IsTrackable: All "keys" generate key press events when they are first pressed. Our other
  //               functionality (IsKeyDown, GetKeyPressAmount, and repeat events) requires being
  //               able to track the key's. This tests whether we can track a given key's state.
  //               Currently, only Pause fails this test.
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

// Key constants - keyboard
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
const int kNumKeys = 300;

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
const int kJoystickNumKeys = kJoystickButtonEnd;

// Joystick key creators
inline GlopKey GetJoystickUp(int joystick) {return GlopKey(joystick, kJoystickAxisUp);}
inline GlopKey GetJoystickRight(int joystick) {return GlopKey(joystick, kJoystickAxisRight);}
inline GlopKey GetJoystickDown(int joystick) {return GlopKey(joystick, kJoystickAxisDown);}
inline GlopKey GetJoystickLeft(int joystick) {return GlopKey(joystick, kJoystickAxisLeft);}
inline GlopKey GetJoystickAxisPos(int joystick, int axis) {
  return GlopKey(joystick, kJoystickAxisPos + 2*axis);
}
inline GlopKey GetJoystickAxisNeg(int joystick, int axis) {
  return GlopKey(joystick, kJoystickAxisNeg + 2*axis);
}
inline GlopKey GetJoystickHatUp(int joystick, int hat) {
  return GlopKey(joystick, kJoystickHatUp + hat*4);
}
inline GlopKey GetJoystickHatRight(int joystick, int hat) {
  return GlopKey(joystick, kJoystickHatRight + hat*4);
}
inline GlopKey GetJoystickHatDown(int joystick, int hat) {
  return GlopKey(joystick, kJoystickHatDown + hat*4);
}
inline GlopKey GetJoystickHatLeft(int joystick, int hat) {
  return GlopKey(joystick, kJoystickHatLeft + hat*4);
}
inline GlopKey GetJoystickButton(int joystick, int button) {
  return GlopKey(joystick, kJoystickButtonStart + button);
}

// KeyEvent
// ========
//
// See comment at the top of the file.
struct KeyEvent {
  enum Type {Nothing, Release, ReleaseDefocus, Press, RepeatPress, DoublePress};
  KeyEvent(const GlopKey &_key, Type type): key(_key), type_(type) {}

  GlopKey key;
  bool IsNothing() const {return type_ == Nothing;}
  bool IsRelease() const {return type_ == Release || type_ == ReleaseDefocus;}
  bool IsReleaseDefocus() const {return type_ == ReleaseDefocus;}
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
  virtual void OnKeyEvent(const KeyEvent &event, int dt) = 0;
  void BeginKeyListening();
  void StopKeyListening();

 private:
  LightSetId listener_id_;
  friend class Input;
  DISALLOW_EVIL_CONSTRUCTORS(KeyListener);
};

// Input
// =====

// Static accessor convenience method
Input *input();

class Input {
 public:
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
    const KeyInfo *info = LocateKeyInfo(key);
    return (info == 0? 0.0f : info->press_amount_now);
  }
  bool IsKeyDownNow(const GlopKey &key) const {
    const KeyInfo *info = LocateKeyInfo(key);
    return (info == 0? false : info->is_down_now);
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

  // Queries whether a particular key was down this frame, or how far down it was. See comment at
  // the top of the file.
  float GetKeyPressAmountFrame(const GlopKey &key) const {
    const KeyInfo *info = LocateKeyInfo(key);
    return (info == 0? 0.0f : info->press_amount_frame);
  }
  bool IsKeyDownFrame(const GlopKey &key) const {
    const KeyInfo *info = LocateKeyInfo(key);
    return (info == 0? false : info->is_down_frame);
  }
  const vector<GlopKey> &GetDownKeysFrame() const {return down_keys_frame_;}

  // Was a key event generated for this key during this frame? WasKeyPressed counts repeat events
  // only if count_repeats == true.
  bool WasKeyPressed(const GlopKey &key, bool count_repeats = true) const {
    const KeyInfo *info = LocateKeyInfo(key);
    return (info == 0? false : count_repeats? info->was_pressed : info->was_pressed_no_repeats); 
  }
  bool WasKeyReleased(const GlopKey &key) const {
    const KeyInfo *info = LocateKeyInfo(key);
    return (info == 0? false : info->was_released);
  }

  // Returns a single key that has been pressed this frame (including repeat events). If no key
  // has been pressed, kNoKey is returned. accept_clicks can be used to filter out mouse clicks,
  // accept_modifiers and accept_motion filter out modifier and motion keys respectively/
  const GlopKey &GetKeyPress(bool accept_clicks = true, bool accept_modifiers = false,
                             bool accept_motion = false);

  // Calls gSystem->think until GetKeyPress != kNoKey.
  const GlopKey &WaitForKeyPress(bool accept_clicks = true, bool accept_modifiers = false,
                                 bool accept_motion = false);

 private:
  // Interface to GlopWindow
  friend class GlopWindow;
  Input(GlopWindow *window);
  ~Input();
  void StartPolling();
  void StopPolling();
  void Think(bool lost_focus, int dt);

  // Meta-state
  GlopWindow *window_;                            // The window that owns us
  InputPollingThread *polling_thread_;            // For polling the OS input events
  int last_poll_time_;                            // The time at which we last did a poll
  int window_x_, window_y_;                       // Current window coordinates - used for tracking
                                                  //  mouse position. Note this is different from
                                                  //  GlopWindow coordinates, which are valid only
                                                  //  in windowed mode.
  bool is_num_lock_set_, is_caps_lock_set_;       // Are num lock and caps lock on right now?
  float mouse_sensitivity_;                       // Scale for mouse motion events
  int mouse_x_, mouse_y_;                         // The current cusors position in window coords
  bool is_cursor_visible_, os_is_cursor_visible_; // Has the user set the cursor to be shown? And
                                                  //  have we told the Os to actually show it?
  int num_joysticks_;                             // The number of joysticks we have detected
  int joystick_refresh_time_;                     // The length of time since we last did a joystick
                                                  //  refresh. Used to prevent doing refreshes too
                                                  //  often.
  bool requested_joystick_refresh_;               // Whether the uses wants to do a joystick refresh

  // Key status
  struct KeyInfo {
    KeyInfo()
    : press_amount_now(0),
      press_amount_frame(0),
      is_down_now(false),
      is_down_frame(false),
      release_delay(0),
      is_press_time_valid(false),
      press_time(0),
      was_pressed(false),
      was_pressed_no_repeats(false),
      was_released(false) {}
    float press_amount_now,      // Instantaneous press_amount for the key
          press_amount_frame;    // Total press_amount for the key this frame
    bool is_down_now,            // Is the key down now - not quite the same as cur_press_amount > 0
         is_down_frame;          // Was the key down at any point this frame (See IsKeyDown)
    int release_delay;           // Minimum time until cur_is_down can be reverted to false
    bool is_press_time_valid;    // Used for tracking double presses
    int press_time;              // "                              "
    bool was_pressed,            // Was a key press event generated for this key this frame
         was_pressed_no_repeats, // Was a key press (no repeat) even generated this frame
         was_released;           // Was a key release event generated for this key this frame
  };
  vector<GlopKey> down_keys_frame_, pressed_keys_frame_;
  KeyInfo keyboard_state_[kNumKeys];
  vector<vector<KeyInfo> > joystick_state_;

  // Key event status
  struct RepeatEvent {
    GlopKey key;
    int delay;
  };
  LightSet<KeyListener*> key_listeners_;
  LightSet<RepeatEvent> repeat_events_;
  
  // Utility functions
  void OnKeyEvent(const KeyEvent &event, int dt);
  KeyInfo *LocateKeyInfo(const GlopKey &key);
  const KeyInfo *LocateKeyInfo(const GlopKey &key) const;
  void UpdateOsCursorVisibility();
  friend class KeyListener;
  DISALLOW_EVIL_CONSTRUCTORS(Input);
};

#endif // GLOP_INPUT_H__
