// Os-specific functionality. This code is all for internal use within Glop, and it is only sorted
// this way so as to make it as easy as possible to introduce code for a new operating system. A
// client program should be using System, GlopWindow, etc. to achieve the functionality included
// within here.
//
// To add a new operating system:
// 1. Implement Os___.cpp that does the following:
//   - Defines the OsWindowData struct.
//   - Defines all functions in the Os class. Accessor functions will be called approximately once
//     per frame, and mutators will be called at most this often, and will never be called
//     redundantly (e.g. SetTitle to the already existing title).
//   - Glop only supports one window. However, it should be possible to change this at some future
//     time if desired. Therefore, the Os code in particular should not heavily depend on this.
//   - Note that all Os windows and functions will be controlled externally. Therefore, with the
//     exception of objects that are never even exposed outside of Os, all objects are owned
//     externally. Thus, Os should not delete windows automatically for example.
// 2. Add the appropriate includes in OpenGl.h.

#ifndef GLOP_OS_H__
#define GLOP_OS_H__

// Includes
#include "Base.h"
#include "Input.h"
#include <string>
#include <vector>
using namespace std;

// Class declarations
class Image;
struct OsMutex;
struct OsWindowData;

// Os class definition
class Os {
 public:
  // Initializes the Os internals.
  static void Init();

  // Shuts the Os internals down. This will be called automatically on program exit.
  static void ShutDown();

  // Logic functions
  // ===============

  // The Glop pipeline for a single frame as seen by the Os is as follows:
  //   - Os::Think is called from the main thread.
  //   - For each window (just one right now):
  //     - Os::WindowThink is called from the main thread.
  //     - All Os accessors for a frame are called from the main thread.
  //
  // Think and WindowThink have two responsibilities:
  //  - Exit the program if the window is closed by the user. This should NOT happen if the window
  //    is closed programatically, which will happen if we switch into or out of fullscreen.
  //  - Make sure all windows we create behave well within the context of the operating system.
  //    For example, if we alt-tab away from a fullscreen program, this should set the resolution
  //    back to the desktop resolution while the window is minimized.
  static void Think();
  static void WindowThink(OsWindowData *window); 

  // Window functions
  // ================

  // Creates an OpenGl window with the given properties. On success, a new OsWindowObject should
  // be returned. On failure, NULL should be returned. Notes:
  //  - icon may be NULL, in which case a default icon should be used.
  //  - is_resizable only applies in the case of a non-fullscreen window.
  static OsWindowData* CreateWindow(const string &title, int x, int y, int width, int height,
                                    bool full_screen, short stencil_bits, const Image *icon,
                                    bool is_resizable);

  // Destroys the given window. Note the window could be created later with a call to CreateWindow.
  // The OsWindowData object should be deleted.
  static void DestroyWindow(OsWindowData *window);

  // Returns whether the given window has been minimized.
  static bool IsWindowMinimized(const OsWindowData *window);

  // Returns whether the given window currently has input focus, and whether it lost input focus
  // at any point since the last call to this function. It is important to track the latter
  // separately so that we can clear the key state even if the window loses and gains focus within
  // a single frame. In Windows, this could easily happen on a call to MessageBox.
  static void GetWindowFocusState(OsWindowData *window, bool *is_in_focus, bool *focus_changed);

  // Returns the on-screen position of the given window. Specifically, x and y gives the screen
  // coordinates of (0, 0) on the window.
  static void GetWindowPosition(const OsWindowData *window, int *x, int *y);

  // Returns the current pixel width and height of the given window.
  static void GetWindowSize(const OsWindowData *window, int *width, int *height);

  // Changes the title for the window to match the given title.
  static void SetTitle(OsWindowData *window, const string &title);

  // Changes the icon for the window to match the given icon.
  // icon may be NULL, in which case a default icon should be used.
  // The image may be of any size and type - the Os class must scale it as required.
  // HEY THIS INTERFACE SUCKS
  // SERIOUSLY IT IS REALLY BAD
  // Talk to Zorba about making it unsucky
  static void SetIcon(OsWindowData *window, const Image *icon);

  // Changes the size of a non full-screen window.
  static void SetWindowSize(OsWindowData *window, int width, int height);

  // Input functions
  // ===============

  // An input event, as generated by Os. See GetInputEvents below.
  struct KeyEvent {
    // A button is pressed or released (special case of the following event).
    // - timestamp gives the time in milliseconds when this event occurred. Only differences
    //   between times are ever considered, so 0 can be based anywhere, and it does not need to
    //   align with System::GetTime().
    // - cursor_x and cursor_y give the mouse position in screen coordinates when this event
    //   occurred.
    // - is_num_lock_set and is_caps_lock_set state whether num lock and caps lock were on when
    //   the event occurred.
    KeyEvent(const GlopKey &_key, bool is_pressed, int _timestamp, int _cursor_x, int _cursor_y,
             bool _is_num_lock_set, bool _is_caps_lock_set)
    : key(_key), press_amount(is_pressed? 1.0f : 0.0f), mouse_dx(0), mouse_dy(0),
      timestamp(_timestamp), cursor_x(_cursor_x), cursor_y(_cursor_y),
      is_num_lock_set(_is_num_lock_set), is_caps_lock_set(_is_caps_lock_set) {}

    // A button's press amount changes. See above.
    KeyEvent(const GlopKey &_key, float _press_amount, int _timestamp, int _cursor_x,
             int _cursor_y, bool _is_num_lock_set, bool _is_caps_lock_set)
    : key(_key), press_amount(_press_amount), mouse_dx(0), mouse_dy(0), timestamp(_timestamp),
      cursor_x(_cursor_x), cursor_y(_cursor_y), is_num_lock_set(_is_num_lock_set),
      is_caps_lock_set(_is_caps_lock_set) {}

    // The mouse moved. See above.
    // mouse_dx and mouse_dy are in mickeys with right and down considered positive.
    KeyEvent(int _mouse_dx, int _mouse_dy, int _timestamp, int _cursor_x,
             int _cursor_y, bool _is_num_lock_set, bool _is_caps_lock_set)
    : key(kNoKey), press_amount(0), mouse_dx(_mouse_dx), mouse_dy(_mouse_dy),
      timestamp(_timestamp), cursor_x(_cursor_x), cursor_y(_cursor_y),
      is_num_lock_set(_is_num_lock_set), is_caps_lock_set(_is_caps_lock_set) {}

    // A dummy event, giving the current input state.
    KeyEvent(int _timestamp, int _cursor_x, int _cursor_y, bool _is_num_lock_set,
             bool _is_caps_lock_set)
    : key(kNoKey), press_amount(0), mouse_dx(0), mouse_dy(0),
      timestamp(_timestamp), cursor_x(_cursor_x), cursor_y(_cursor_y),
      is_num_lock_set(_is_num_lock_set), is_caps_lock_set(_is_caps_lock_set) {}

    GlopKey key;
    float press_amount;
    int mouse_dx, mouse_dy;
    int timestamp, cursor_x, cursor_y;
    bool is_num_lock_set, is_caps_lock_set;
  };

  // Returns all user input events that have occured this frame. The events should be returned in
  // the order in which they occurred. This function will be called exactly once per frame.
  // - Redundant events may be generated. For example, it is okay to repeatedly state that a key
  //   has been pressed while it is held down. These events are never necessary however.
  // - Derived keys (eg. kKeyLeftShift or anything with kDeviceAnyJoystick) should never have
  //   events generated for them. That is done in Input.
  // There should always be a dummy event at the end of the event list giving the current input
  // state (timestamp, cursor pos, num lock & caps lock).
  static vector<KeyEvent> GetInputEvents(OsWindowData *window);

  // Warps the mouse cursor to the given screen coordinates (NOT window coordinates).
  static void SetMousePosition(int x, int y);

  // Switches the mouse cursor to be visible/invisible. Glop will ensure that if the cursor leaves
  // the window area, or the window loses focus, the mouse cursor will be explicitly made visible.
  // Thus, Os::ShowMouseCursor can either work globally or just over the window in question. Also,
  // Glop will ensure this function is not called redundantly.
  static void ShowMouseCursor(bool is_shown);

  // Locks the mouse cursor to a window.
  static void LockMouseCursor(OsWindowData *window);
  
  // Updates the given window so that its joystick information is completely up to date (i.e.,
  // if a user plugs in a joystick, RefreshJoysticks should update data based on the change). This
  // should be reflected both in GetNumJoysticks() and in joystick notifications from System. If
  // the joystick information has not changed, this should NOT disrupt regular input polling.
  // Note: this kind of refresh should not happen automatically, except possibly on window
  // re-creation where it is difficult to avoid.
  static void RefreshJoysticks(OsWindowData *window);

  // Returns the number of joysticks that we are currently aware of.
  static int GetNumJoysticks(OsWindowData *window);

  // File system functions
  // =====================

  // Returns all files in the given directory. Hidden files should be ignored.
  static vector<string> ListFiles(const string &directory);

  // Returns all subdirectories of the given directory. Hidden directories should be ignored.
  static vector<string> ListSubdirectories(const string &directory);

  // Threading functions
  // ===================

  // Creates a new thread that starts executing thread_function with the parameter data.
  static void StartThread(void(*thread_function)(void *), void *data);

  // Create and delete a mutex. The mutex should NOT be acquired on creation.
  static OsMutex *NewMutex();
  static void DeleteMutex(OsMutex *mutex);

  // Acquire and release a mutex. If a mutex is already acquired, another call to AcquireMutex
  // should block until the mutex is released. (This is the sole purpose of a mutex.)
  static void AcquireMutex(OsMutex *mutex);
  static void ReleaseMutex(OsMutex *mutex);

  // Miscellaneous functions
  // =======================

  // Displays a modal message box. This should work even if no window can be initialized.
  // The title may or may not be displayed with the message, depending on the operating system.
  static void MessageBox(const string &title, const string &message);

  // Returns a list of all 32 bpp fullscreen video modes that are supported by this computer. The
  // modes should be listed in increasing lexicographical order of pixel size: (width, height).
  static vector<pair<int,int> > GetFullScreenModes();

  // Gives up context for the current thread for t milliseconds.
  static void Sleep(int t);

  // Returns the number of milliseconds that have elapsed since some unspecified basepoint.
  static int GetTime();
  
  // Returns the number of microseconds that have elapsed since some unspecified basepoint.
  static int64 GetTimeMicro();

  // Returns the refresh rate for the primary display.
  static int GetRefreshRate();

  // Turns on or off video sync for the current window. See GlopWindow.
  static void EnableVSync(bool is_enabled);

  // Switches Open Gl buffers so that all rendering to the back buffer now appears on the screen.
  static void SwapBuffers(OsWindowData *window);

  // Sets the window as the current OpenGL context. All rendering done after this call will be
  // applied to this window. This is not used currently but it will be important if Glop switches
  // to support of multiple windows. Not yet implemented on Win32.
  static void SetCurrentContext(OsWindowData* window);
};

#endif // GLOP_OS_H__
