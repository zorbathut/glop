// Os-specific functionality. This code is all for internal use within Glop, and it is only sorted
// this way so as to make it as easy as possible to introduce code for a new operating system. A
// client program should be using System, GlopWindow, etc. to achieve the functionality included
// within here.
//
// To add a new operating system:
// 1. Implement Os___.cpp that does the following:
//   - Defines the main function, which calls GlopInternalMain(int argnum, char **argv) when done.
//     Here, argv[0] should be the executable that is being run, and argv[] should be the
//     full list of parameters, parsed in the same way as g++.
//   - Defines the OsWindowData struct.
//   - Defines all functions in the Os class. Accessor functions will be called approximately once
//     per frame, and mutators will be called at most this often, and will never be called
//     redundantly (e.g. SetTitle to the already existing title).
//   - Glop only supports one window. However, it should be possible to change this at some future
//     time if desired. Therefore, the Os code in particular should not heavily depend on this.
// 2. Add the appropriate includes in OpenGl.h.

#ifndef GLOP_Os_H__
#define GLOP_Os_H__

// Includes
#include "Base.h"
#include "Input.h"
#include <vector>
using namespace std;

// Class declarations
class Image;
struct OsMutex;
struct OsWindowData;

// Os class definition
class Os {
 public:
   // TODO(jwills): discuss with darthur about how this should be handled.
   static void Init();
   
  // Logic functions
  // ===============

  // The Glop pipeline for a single frame as seen by the Os is as follows:
  //   - Os::Think is called from the main thread
  //   - For each window (just one right now):
  //     - Os::WindowThink is called from the main thread
  //     - All Os accessors for a frame are called from the main thread
  // One notable exception is that Os::PollEvent will be called periodically from a helper thread.
  // It will never execute at the same time as: RefreshJoysticks, CreateWindow, or DestroyWindow.
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

  // Creates an Open Gl window with the given properties. On success, a new OsWindowObject should
  // be returned. On failure, NULL should be returned. Notes:
  //  - icon may be NULL, in which case a default icon should be used.
  //  - is_resizable only applies in the case of a non-fullscreen window.
  static OsWindowData* CreateWindow(const string &title, int x, int y, int width, int height,
                                    bool full_screen, short stencil_bits, const Image *icon,
                                    bool is_resizable);

  // Destroys the given window. Note the window could be created later with a call to CreateWindow.
  // The OsWindowData object should be deleted.
  static void DestroyWindow(OsWindowData *window);

  // Sets the window as the current OpenGL context.  All rendering done after this call will be
  // applied to this window
  // TODO(jwills): SetCurrentContext isn't implemented yet on Win32Glop
  static void SetCurrentContext(OsWindowData* window);

  // Returns whether the given window has been minimized.
  static bool IsWindowMinimized(const OsWindowData *window);

  // Returns whether the given window currently has input focus, and whether it lost input focus
  // at any point since the last call to this function. It is important to track the latter
  // separately so that we can clear the key state even if the window loses and gains focus within
  // a single frame. In Windows, this could easily happen on a call to DisplayMessage.
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
  static void SetIcon(OsWindowData *window, const Image *icon);

  // Input functions
  // ===============

  // Returns all user input that has occured since the last call to PollInput. Note that this is
  // happening in a separate thread from other Os calls. See above for a discussion of safety
  // guarantees.
  //
  // Format:
  //  - is_num_lock_set and is_caps_lock_set should be filled with the current keyboard settings.
  //  - cursor_x and cursor_y should be filled with the screen (i.e., not window) coordinates for
  //    the mouse cursor.
  //  - mouse_dx and mouse_dx should be filled with the number of mickeys that the mouse has moved
  //    since the last call in the x and y direction (right and down are considered positive). This
  //    should work even if the cursor is at the edge of the screen.
  //  - State changes for all keys other than mouse motion should be returned in the
  //    vector<KeyEvent> (including joystick axes, mouse wheel, etc.):
  //    - Keyboard key events should be in the same order in which they happened
  //    - Redundant events (e.g. a key down event when a key is already down) may be returned, but
  //      they will be ignored. Repeat events, as seen by the Glop user, are generated elsewhere.
  //    - press_amount may be negative in KeyEvents. This is equivalent to 0.
  struct KeyEvent {
    KeyEvent(const GlopKey &_key, bool is_pressed)
      : key(_key), press_amount(is_pressed? 1.0f : 0.0f) {}
    KeyEvent(const GlopKey &_key, float _press_amount): key(_key), press_amount(_press_amount) {}
    GlopKey key;
    float press_amount;
  };
  static vector<KeyEvent> PollInput(OsWindowData *window, bool *is_num_lock_set,
                                    bool *is_caps_lock_set, int *cursor_x, int *cursor_y,
                                    int *mouse_dx, int *mouse_dy);

  // Warps the mouse cursor to the given screen coordinates (NOT window coordinates).
  static void SetMousePosition(int x, int y);

  // Switches the mouse cursor to be visible/invisible. Glop will ensure that if the cursor leaves
  // the window area, or the window loses focus, the mouse cursor will be explicitly made visible.
  // Thus, Os::ShowMouseCursor can either work globally or just over the window in question. Also,
  // Glop will ensure this function is not called redundantly.
  static void ShowMouseCursor(bool is_shown);

  // Updates the given window so that its joystick information is completely up to date (i.e.,
  // if a user plugs in a joystick, RefreshJoysticks should update data based on the change). This
  // should be reflected both in GetNumJoysticks() and in joystick notifications from System. If
  // the joystick information has not changed, this should NOT disrupt regular input polling.
  // Note: this refresh should not happen automatically, except possibly on window recreation where
  // it is difficult to avoid.
  static void RefreshJoysticks(OsWindowData *window);

  // Returns the number of joysticks that we are currently aware of.
  static int GetNumJoysticks(OsWindowData *window);

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

  // Displays a message in a way that should get the user's attention, as might be suitable for
  // a fatal error message. This should work even if no window can be initialized.
  // The title may or may not be displayed with the message, depending on the operating system.
  static void DisplayMessage(const string &title, const string &message);

  // Returns a list of all 32 bpp fullscreen video modes that are supported by this computer. The
  // modes should be listed in increasing lexicographical order of pixel size: (width, height).
  static vector<pair<int, int> > GetFullScreenModes();

  // Gives up context for the current thread for t milliseconds.
  static void Sleep(int t);

  // Returns the number of milliseconds that have elapsed since some unspecified basepoint.
  static int GetTime();

  // Switches Open Gl buffers so that all rendering to the back buffer now appears on the screen.
  static void SwapBuffers(OsWindowData *window);
};

#endif // GLOP_Os_H__
