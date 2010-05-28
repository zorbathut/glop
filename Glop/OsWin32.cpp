#ifdef WIN32

// Includes
#include "Os.h"
#include "Image.h"
#include "System.h"
#include "Thread.h"
#include "GlopWindow.h"
#define DIRECTINPUT_VERSION 0x0700
#include "dinput.h"
#include <process.h>
#include <map>
#include <set>
#include <vector>
#include <windows.h>
#include <gl/gl.h>
using namespace std;

// Do not show the console window for a console application running in release mode
#ifdef NDEBUG
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif

// Undefines, because Windows sucks
#undef CreateWindow
#undef MessageBox

// OsMutex struct definition
struct OsMutex {
  CRITICAL_SECTION critical_section;
};

// OsWindowData struct definition
class InputPollingThread;
struct OsWindowData {
  OsWindowData()
  : icon_handle(0), window_handle(0), device_context(0), rendering_context(0), direct_input(0),
    keyboard_device(0), mouse_device(0), input_polling_thread(0), is_full_screen(0), x(0), y(0),
    width(0), height(0), is_in_focus(false), focus_changed(false), is_minimized(false) {}

  // Operating system values and handles. icon_handle is only non-zero if it will need to be
  // deleted eventually.
  HICON icon_handle;
  HWND window_handle;
	HDC device_context;
	HGLRC rendering_context;
  LPDIRECTINPUT direct_input;
  LPDIRECTINPUTDEVICE keyboard_device, mouse_device;
  vector<LPDIRECTINPUTDEVICE2> joystick_devices;
  InputPollingThread *input_polling_thread;
  Mutex input_mutex;

  // Queriable window properties
  bool is_full_screen;
  int x, y;
  int width, height;
  bool is_in_focus, focus_changed, is_minimized;
};

// Constants
const int kBpp = 32;
const int kDirectInputBufferSize = 50;
const int kJoystickAxisRange = 10000;
const GlopKey kDIToGlopKeyIndex[] = {0,
  27, '1', '2', '3', '4',
  '5', '6', '7', '8', '9',
  '0', '-', '=', 8, 9,
  'q', 'w', 'e', 'r', 't',
  'y', 'u', 'i', 'o', 'p',
  '[', ']', 13, kKeyLeftControl, 'a',
  's', 'd', 'f', 'g', 'h',
  'j', 'k', 'l', ';', '\'',
  '`', kKeyLeftShift, '\\', 'z', 'x',
  'c', 'v', 'b', 'n', 'm',                                 // 50
  ',', '.', '/', kKeyRightShift, kKeyPadMultiply,
  kKeyLeftAlt, ' ', kKeyCapsLock, kKeyF1, kKeyF2,
  kKeyF3, kKeyF4, kKeyF5, kKeyF6, kKeyF7,
  kKeyF8, kKeyF9, kKeyF10, kKeyNumLock, kKeyScrollLock,
  kKeyPad7, kKeyPad8, kKeyPad9, kKeyPadSubtract, kKeyPad4,
  kKeyPad5, kKeyPad6, kKeyPadAdd, kKeyPad1, kKeyPad2,
  kKeyPad3, kKeyPad0, kKeyPadDecimal, -1, -1,
  -1, kKeyF11, kKeyF12, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,                                      // 100
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,                                      // 150
  -1, -1, -1, -1, -1,
  kKeyPadEnter, kKeyRightControl, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  kKeyPadDivide, -1, kKeyPrintScreen, kKeyRightAlt, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, kKeyPause, -1, kKeyHome, kKeyUp,                     // 200
  kKeyPageUp, -1, kKeyLeft, -1, kKeyRight,
  -1, kKeyEnd, kKeyDown, kKeyPageDown, kKeyInsert,
  kKeyDelete, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1,                                      // 250
  -1, -1, -1, -1, -1};

// Globals
static LARGE_INTEGER gTimerFrequency;
static map<HWND, OsWindowData*> gWindowMap;
  
HWND get_first_handle() {
  ASSERT(gWindowMap.size());
  return gWindowMap.begin()->first;
}

// InputPollingThread
// ==================
//
// A separate thread devoted entirely to polling the input device state at regular intervals. This
// is necessitated on Windows because joystick event trapping seems not to work. By polling in a
// separate thread, we guarantee fast response times even when the program's frame rate lags.
class InputPollingThread: public Thread {
 public:
  // Constructor.
  InputPollingThread(OsWindowData *window): window_(window) {}

  // Returns all events since the last call to GetData.
  vector<Os::KeyEvent> GetData() {
    // Get the data
    window_->input_mutex.Acquire();
    vector<Os::KeyEvent> result = data_;
    data_.clear();
    window_->input_mutex.Release();

    // Add a current state event
 	  POINT cursor_pos;
	  GetCursorPos(&cursor_pos);
    bool is_num_lock_set = (GetKeyState(VK_NUMLOCK) & 1) > 0;
    bool is_caps_lock_set = (GetKeyState(VK_CAPITAL) & 1) > 0;
    result.push_back(Os::KeyEvent(system()->GetTime(), cursor_pos.x, cursor_pos.y, is_num_lock_set,
                                  is_caps_lock_set));
    return result;
  }

 protected:
  // Continuously polls the input.
  void Run() {
    while (!IsStopRequested()) {
      window_->input_mutex.Acquire();
      int timestamp = system()->GetTime();

      // Read metastate
 	    POINT cursor_pos;
	    GetCursorPos(&cursor_pos);
      bool is_num_lock_set = (GetKeyState(VK_NUMLOCK) & 1) > 0;
      bool is_caps_lock_set = (GetKeyState(VK_CAPITAL) & 1) > 0;

      // Read keyboard events
      DWORD num_items = kDirectInputBufferSize;
      DIDEVICEOBJECTDATA buffer[kDirectInputBufferSize];
      HRESULT hr = window_->keyboard_device->GetDeviceData(sizeof(buffer[0]), buffer,
                                                           &num_items, 0);
      if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
        window_->keyboard_device->Acquire();
        hr = window_->keyboard_device->GetDeviceData(sizeof(buffer[0]), buffer, &num_items, 0);
      }
      if (!FAILED(hr)) {
        for (int i = 0; i < (int)num_items; i++) {
          bool is_pressed = ((buffer[i].dwData & 0x80) > 0);
          if (buffer[i].dwOfs < 255 && kDIToGlopKeyIndex[buffer[i].dwOfs] != -1)
            data_.push_back(Os::KeyEvent(kDIToGlopKeyIndex[buffer[i].dwOfs], is_pressed, timestamp,
                                         cursor_pos.x, cursor_pos.y, is_num_lock_set,
                                         is_caps_lock_set));
        }
      }

      // Read the mouse state
      DIMOUSESTATE2 mouse_state;
      hr = window_->mouse_device->GetDeviceState(sizeof(mouse_state), &mouse_state);
      if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
        window_->mouse_device->Acquire(); 
        hr = window_->mouse_device->GetDeviceState(sizeof(mouse_state), &mouse_state);
      }
      if (!FAILED(hr)) {
        data_.push_back(Os::KeyEvent(mouse_state.lX, mouse_state.lY, timestamp, cursor_pos.x,
                                     cursor_pos.y, is_num_lock_set, is_caps_lock_set));
        data_.push_back(Os::KeyEvent(kMouseWheelDown, mouse_state.lZ < 0, timestamp, cursor_pos.x,
                                     cursor_pos.y, is_num_lock_set, is_caps_lock_set));
        data_.push_back(Os::KeyEvent(kMouseWheelUp, mouse_state.lZ > 0, timestamp, cursor_pos.x,
                                     cursor_pos.y, is_num_lock_set, is_caps_lock_set));
        ASSERT(kNumMouseButtons == 8);  // Update section if this changes
        for (int i = 0; i < kNumMouseButtons; i++) {
          data_.push_back(Os::KeyEvent(GetMouseButton(i), (mouse_state.rgbButtons[i] & 0x80) > 0,
                                       timestamp, cursor_pos.x, cursor_pos.y, is_num_lock_set,
                                       is_caps_lock_set));
        }
      }

      // Read the joystick states
      DIJOYSTATE2 joy_state;
      for (int i = 0; i < (int)window_->joystick_devices.size(); i++) {
        // Try to poll the device
        hr = window_->joystick_devices[i]->Poll();
        if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
          window_->joystick_devices[i]->Acquire();
          hr = window_->joystick_devices[i]->Poll();
        }
        if (FAILED(window_->joystick_devices[i]->GetDeviceState(sizeof(joy_state), &joy_state)))
          continue;

        // Read axis data
        ASSERT(kNumJoystickAxes == 6);  // Update section if this changes
        data_.push_back(Os::KeyEvent(GetJoystickRight(i), float(joy_state.lX)/kJoystickAxisRange,
                                     timestamp, cursor_pos.x, cursor_pos.y, is_num_lock_set,
                                     is_caps_lock_set));
        data_.push_back(Os::KeyEvent(GetJoystickLeft(i), float(-joy_state.lX)/kJoystickAxisRange,
                                     timestamp, cursor_pos.x, cursor_pos.y, is_num_lock_set,
                                     is_caps_lock_set));
        data_.push_back(Os::KeyEvent(GetJoystickUp(i), float(-joy_state.lY)/kJoystickAxisRange,
                                     timestamp, cursor_pos.x, cursor_pos.y, is_num_lock_set,
                                     is_caps_lock_set));
        data_.push_back(Os::KeyEvent(GetJoystickDown(i), float(joy_state.lY)/kJoystickAxisRange,
                                     timestamp, cursor_pos.x, cursor_pos.y, is_num_lock_set,
                                     is_caps_lock_set));
        data_.push_back(Os::KeyEvent(GetJoystickAxisPos(2, i),
                                     float(joy_state.lZ) / kJoystickAxisRange,
                                     timestamp, cursor_pos.x, cursor_pos.y, is_num_lock_set,
                                     is_caps_lock_set));
        data_.push_back(Os::KeyEvent(GetJoystickAxisNeg(2, i),
                                     float(-joy_state.lZ) / kJoystickAxisRange,
                                     timestamp, cursor_pos.x, cursor_pos.y, is_num_lock_set,
                                     is_caps_lock_set));
        data_.push_back(Os::KeyEvent(GetJoystickAxisPos(3, i),
                                     float(joy_state.lRz) / kJoystickAxisRange,
                                     timestamp, cursor_pos.x, cursor_pos.y, is_num_lock_set,
                                     is_caps_lock_set));
        data_.push_back(Os::KeyEvent(GetJoystickAxisNeg(3, i),
                                     float(-joy_state.lRz) / kJoystickAxisRange,
                                     timestamp, cursor_pos.x, cursor_pos.y, is_num_lock_set,
                                     is_caps_lock_set));
        data_.push_back(Os::KeyEvent(GetJoystickAxisPos(4, i),
                                     float(joy_state.lRx) / kJoystickAxisRange,
                                     timestamp, cursor_pos.x, cursor_pos.y, is_num_lock_set,
                                     is_caps_lock_set));
        data_.push_back(Os::KeyEvent(GetJoystickAxisNeg(4, i),
                                     float(-joy_state.lRx) / kJoystickAxisRange,
                                     timestamp, cursor_pos.x, cursor_pos.y, is_num_lock_set,
                                     is_caps_lock_set));
        data_.push_back(Os::KeyEvent(GetJoystickAxisPos(5, i),
                                     float(joy_state.lRy) / kJoystickAxisRange,
                                     timestamp, cursor_pos.x, cursor_pos.y, is_num_lock_set,
                                     is_caps_lock_set));
        data_.push_back(Os::KeyEvent(GetJoystickAxisNeg(5, i),
                                     float(-joy_state.lRy) / kJoystickAxisRange,
                                     timestamp, cursor_pos.x, cursor_pos.y, is_num_lock_set,
                                     is_caps_lock_set));
    
        // Read hat data
        ASSERT(kNumJoystickHats <= 4);  // Update section if this changes
        for (int j = 0; j < kNumJoystickHats; j++) {
          int angle = joy_state.rgdwPOV[j];
          float hx = 0, hy = 0;
          if (LOWORD(angle) != 0xFFFF) {
            if (angle < 4500) hx = float(angle) / 4500;
            else if (angle <= 13500) hx = 1;
            else if (angle < 22500) hx = 1 - float(angle-13500) / 4500;
            else if (angle <= 31500) hx = -1;
            else hx = -1 + float(angle-31500) / 4500;
            if (angle < 4500) hy = 1;
            else if (angle <= 13500) hy = 1 - float(angle-4500) / 4500;
            else if (angle < 22500) hy = -1;
            else if (angle <= 31500) hy = -1 + float(angle-22500) / 4500;
            else hy = 1;
          }
          data_.push_back(Os::KeyEvent(GetJoystickHatUp(j, i), hy, timestamp, cursor_pos.x,
                                       cursor_pos.y, is_num_lock_set, is_caps_lock_set));
          data_.push_back(Os::KeyEvent(GetJoystickHatRight(j, i), hx, timestamp, cursor_pos.x,
                                       cursor_pos.y, is_num_lock_set, is_caps_lock_set));
          data_.push_back(Os::KeyEvent(GetJoystickHatDown(j, i), -hy, timestamp, cursor_pos.x,
                                       cursor_pos.y, is_num_lock_set, is_caps_lock_set));
          data_.push_back(Os::KeyEvent(GetJoystickHatLeft(j, i), -hx, timestamp, cursor_pos.x,
                                       cursor_pos.y, is_num_lock_set, is_caps_lock_set));
        }

        // Read button data
        ASSERT(kNumJoystickButtons <= 128);  // Update section if this changes
        for (int j = 0; j < kNumJoystickButtons; j++) {
          bool is_pressed = ((joy_state.rgbButtons[j] & 0x80) > 0);
          data_.push_back(Os::KeyEvent(GetJoystickButton(j, i), is_pressed, timestamp,
                                        cursor_pos.x, cursor_pos.y, is_num_lock_set,
                                        is_caps_lock_set));
        }
      }

      window_->input_mutex.Release();
      Os::Sleep(10);
    }
  }

  vector<Os::KeyEvent> data_;
  OsWindowData *window_;
  DISALLOW_EVIL_CONSTRUCTORS(InputPollingThread);
};

// Initialization/Shut down
// ========================

void Os::Init() {
  for (map<HWND, OsWindowData*>::iterator it = gWindowMap.begin(); it != gWindowMap.end(); it++)
    it->second->input_mutex.Acquire();

  // Timer initialization. timeBeginPeriod(1) ensures that Sleep calls return promptly when they
  // are supposed to, and QueryPerformanceFrequency is needed for Os::GetTime.
  timeBeginPeriod(1);
  QueryPerformanceFrequency(&gTimerFrequency);

  for (map<HWND, OsWindowData*>::iterator it = gWindowMap.begin(); it != gWindowMap.end(); it++)
    it->second->input_mutex.Release();
}

void Os::ShutDown() {}

// Logic functions
// ===============

// Handles Os messages that arrive through the message queue. Note that some, but not all, messages
// are sent directly to HandleMessage and bypass the message queue.
void Os::Think() {
  MSG message;
  while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&message);
    DispatchMessage(&message);
  }
}

// Handles window messages that arrive by any means, message queue or by direct notification.
// However, key events are ignored, as input is handled by DirectInput in WindowThink().
LRESULT CALLBACK HandleMessage(HWND window_handle, UINT message, WPARAM wparam, LPARAM lparam) {
  // Extract information from the parameters
  if (!gWindowMap.count(window_handle))
    return DefWindowProcW(window_handle, message, wparam, lparam);
  OsWindowData *os_window = gWindowMap[window_handle];
  unsigned short wparam1 = LOWORD(wparam), wparam2 = HIWORD(wparam);
  unsigned short lparam1 = LOWORD(lparam), lparam2 = HIWORD(lparam);

	// Handle each message
  switch (message) {
    case WM_SYSCOMMAND:
      // Prevent screen saver and monitor power saving
      if (wparam == SC_SCREENSAVE || wparam == SC_MONITORPOWER)
        return 0;
      // Prevent accidental pausing by pushing F10 or what not
      if (wparam == SC_MOUSEMENU || wparam == SC_KEYMENU)
        return 0;
      break;
    case WM_CLOSE:
      window()->Destroy();  // this is certainly not going to fail hideously
      return 0;
    case WM_MOVE:
      os_window->x = (signed short)lparam1;
      os_window->y = (signed short)lparam2;
      break;
    case WM_SIZE:
      // Set the resolution if a full-screen window was alt-tabbed into.
      if (os_window->is_minimized != (wparam == SIZE_MINIMIZED) && os_window->is_full_screen) {
        if (wparam == SIZE_MINIMIZED) {
          ChangeDisplaySettings(0, 0);
        } else {
          DEVMODE screen_settings;
	        screen_settings.dmSize = sizeof(screen_settings);
          screen_settings.dmDriverExtra = 0;
	        screen_settings.dmPelsWidth = lparam1;
	        screen_settings.dmPelsHeight = lparam2;
          screen_settings.dmBitsPerPel = kBpp;
          screen_settings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
          ChangeDisplaySettings(&screen_settings, CDS_FULLSCREEN);
        }
      }
      os_window->is_minimized = (wparam == SIZE_MINIMIZED);
      if (!os_window->is_minimized) {
        os_window->width = lparam1;
        os_window->height = lparam2;
      }
      break;
    case WM_SIZING:
      os_window->focus_changed = true;
      break;
	  case WM_ACTIVATE:
      os_window->is_in_focus = (wparam1 == WA_ACTIVE || wparam1 == WA_CLICKACTIVE);
      os_window->focus_changed = true;
      // If the user alt-tabs out of a fullscreen window, the window will keep drawing and will
      // remain in full-screen mode. Here, we minimize the window, which fixes the drawing problem,
      // and then the WM_SIZE event fixes the full-screen problem.
      if (!os_window->is_in_focus && os_window->is_full_screen)
        ShowWindow(os_window->window_handle, SW_MINIMIZE);
      break;
  }

  // Pass on remaining messages to the default handler
  
  return DefWindowProcW(window_handle, message, wparam, lparam);
}

void Os::WindowThink(OsWindowData *window) {}

// Window functions
// ================

// See Os.h

// Converts an image into a 32x32 ICO object in memory, and then returns a handle for the resulting
// icon.
HICON CreateIcon(OsWindowData *data, const Image *image) {
  bool scaling_needed = (image->GetWidth() != 32 || image->GetHeight() != 32 ||
                         image->GetBpp() != 32);
  if (scaling_needed)
    image = Image::AdjustedImage(image, 32, 32, 32);

  // Set the header
  unsigned char *icon = new unsigned char[3240];
  *((unsigned int*)&icon[0]) = 40;
  *((unsigned int*)&icon[4]) = 32;
  *((unsigned int*)&icon[8]) = 64;
  *((unsigned short*)&icon[12]) = 1;
  *((unsigned short*)&icon[14]) = 24;
  *((unsigned int*)&icon[16]) = 0;
  *((unsigned int*)&icon[20]) = 3200;
  *((unsigned int*)&icon[24]) = 0;
  *((unsigned int*)&icon[28]) = 0;
  *((unsigned int*)&icon[32]) = 0;
  *((unsigned int*)&icon[36]) = 0;

  // Set the colors
	for (int y = 0; y < 32; y++)
  for (int x = 0; x < 32; x++) {
    const unsigned char *pixel = image->Get(x, 31-y);
    for (int c = 0; c < 3; c++) {
      unsigned char value = pixel[c];
      if (pixel[3] == 0)
        value = 0;  // Do not do strange background blending
      icon[40 + y*32*3 + x*3 + 2-c] = value;
    }
  }

  // Set the mask using alpha values
  for (int y = 0; y < 32; y++)
  for (int x = 0; x < 32; x++) {
    int icon_index = y*4 + x/8;
    int icon_mask = 1 << (7-(x%8));
    if (image->Get(x, 31-y)[3] == 0)
      icon[3112+icon_index] |= icon_mask;
		else
      icon[3112+icon_index] &= (~icon_mask);
  }

  // Do the work
  HICON result = CreateIconFromResource(icon, 3240, true, 0x00030000);
  delete icon;
  if (scaling_needed)
    delete image;
  return result;
}

OsWindowData *Os::CreateWindow(const string &title, int x, int y,
                               int width, int height, bool full_screen, short stencil_bits,
                               const Image *icon, bool is_resizable) {
  const wchar_t *const kClassName = L"GlopWin32";
  static bool is_class_initialized = false;
  OsWindowData *result = new OsWindowData();
  
  // Create a window class. This is essentially used by Windows to group together several windows
  // for various purposes.
  if (!is_class_initialized) {
    WNDCLASSW window_class;
    window_class.style = CS_OWNDC;
    window_class.lpfnWndProc = HandleMessage;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = GetModuleHandle(0);
    window_class.hIcon = 0;
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = 0;
    window_class.lpszMenuName = 0;
    window_class.lpszClassName = kClassName;
    if (!RegisterClassW(&window_class)) {
      DestroyWindow(result);
      return 0;
    }
    is_class_initialized = true;
  }

  // Specify the desired window style
  DWORD window_style = WS_POPUP;
  if (!full_screen) {
    window_style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    if (is_resizable)
      window_style |= WS_MAXIMIZEBOX | WS_THICKFRAME;
  }

  // Specify the window dimensions and get the border size
  RECT window_rectangle;
  window_rectangle.left = 0;
  window_rectangle.right = width;
  window_rectangle.top = 0;
  window_rectangle.bottom = height;
  if (!AdjustWindowRectEx(&window_rectangle, window_style, false, 0)) {
    DestroyWindow(result);
	  return 0;
  }

	// Specify the desired window position
  if (x == -1 && y == -1) {
    x = y = CW_USEDEFAULT;
  } else if (full_screen) {
    x = y = 0;
  } else {
    x += window_rectangle.left;
    y += window_rectangle.top;
  }

  // Create the window
  result->window_handle = CreateWindowExW(0,
                                         kClassName, L"Glop window",
                                         window_style,
                                         x, y,
                                         window_rectangle.right - window_rectangle.left, 
								                         window_rectangle.bottom - window_rectangle.top,
                                         NULL, 
                                         NULL,
                                         GetModuleHandle(0),
                                         NULL);
  if (!result->window_handle) {
    DestroyWindow(result);
    return 0;
  }
  
  SetTitle(result, title);
  
  gWindowMap[result->window_handle] = result;

  // Set the icon
  if (icon != 0) {
    result->icon_handle = CreateIcon(result, icon);
    SendMessage(result->window_handle, WM_SETICON, ICON_BIG, (LPARAM)result->icon_handle);
  }

  // Get the window position
  RECT actual_position;
  GetWindowRect(result->window_handle, &actual_position);
  result->x = actual_position.left - window_rectangle.left;
  result->y = actual_position.top - window_rectangle.top;
  result->width = width;
  result->height = height;

  // Get the device context
  result->device_context = GetDC(result->window_handle);
  if (!result->device_context) {
    DestroyWindow(result);
    return 0;
  }

  // Specify a pixel format by requesting one, and then selecting the best available match on the
  // system. This is used to set up Open Gl.
  PIXELFORMATDESCRIPTOR pixel_format_request;
  memset(&pixel_format_request, 0, sizeof(pixel_format_request));
  pixel_format_request.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  pixel_format_request.nVersion = 1;
  pixel_format_request.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pixel_format_request.iPixelType = PFD_TYPE_RGBA;
  pixel_format_request.cColorBits = kBpp;
  pixel_format_request.cStencilBits = (char)stencil_bits;
  pixel_format_request.cDepthBits = 16;
  unsigned int pixel_format_id = ChoosePixelFormat(result->device_context, &pixel_format_request);
  if (!pixel_format_id) {
    DestroyWindow(result);
    return 0;
  }
  if (!SetPixelFormat(result->device_context, pixel_format_id, &pixel_format_request)) {
    DestroyWindow(result);
    return 0;
  }

  // Switch to full-screen mode if appropriate
  if (full_screen) {
    DEVMODE screen_settings;
	  screen_settings.dmSize = sizeof(screen_settings);
    screen_settings.dmDriverExtra = 0;
    screen_settings.dmPelsWidth = width;
	  screen_settings.dmPelsHeight = height;
    screen_settings.dmBitsPerPel = kBpp;
    screen_settings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
    if (ChangeDisplaySettings(&screen_settings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
      DestroyWindow(result);
      return 0;
    }
    result->is_full_screen = true;
  }

  // Make a rendering context for this thread
  result->rendering_context = wglCreateContext(result->device_context);
  if (!result->rendering_context) {
    DestroyWindow(result);
    return 0;
  }
  wglMakeCurrent(result->device_context, result->rendering_context);

  // Show the window. Note that SetForegroundWindow can fail if the user is currently using another
  // window, but this is fine and nothing to be alarmed about.
  ShowWindow(result->window_handle, SW_SHOW);
  SetForegroundWindow(result->window_handle);
  SetFocus(result->window_handle);
  result->is_in_focus = true;

  // Attempt to initialize DirectInput.
  // Settings: Non-exclusive (be friendly with other programs), foreground (only accept input
  //                          events if we are currently in the foreground).
  if (FAILED(DirectInputCreate(GetModuleHandle(0), DIRECTINPUT_VERSION,
                               &result->direct_input, 0))) {
    DestroyWindow(result);
    return 0;
  }
  result->direct_input->CreateDevice(GUID_SysKeyboard, &result->keyboard_device, NULL);
  result->keyboard_device->SetCooperativeLevel(result->window_handle,
                                               DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
  result->keyboard_device->SetDataFormat(&c_dfDIKeyboard);
  result->direct_input->CreateDevice(GUID_SysMouse, &result->mouse_device, NULL);
  result->mouse_device->SetCooperativeLevel(result->window_handle,
                                            DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
  result->mouse_device->SetDataFormat(&c_dfDIMouse2);

  // Set the DirectInput buffer size - this is the number of events it can store at a single time
  DIPROPDWORD prop_buffer_size;
  prop_buffer_size.diph.dwSize = sizeof(DIPROPDWORD);
  prop_buffer_size.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  prop_buffer_size.diph.dwObj = 0;
  prop_buffer_size.diph.dwHow = DIPH_DEVICE;
  prop_buffer_size.dwData = kDirectInputBufferSize;
  result->keyboard_device->SetProperty(DIPROP_BUFFERSIZE, &prop_buffer_size.diph);
  RefreshJoysticks(result);

  // Begin the input polling
  result->input_polling_thread = new InputPollingThread(result);
  result->input_polling_thread->Start();

  // All done
  return result;
}

// Destroys a window that is completely or partially created.
void Os::DestroyWindow(OsWindowData *window) {
  window->input_polling_thread->RequestStop();
  window->input_polling_thread->Join();
  delete window->input_polling_thread;
  if (window->is_full_screen && !window->is_minimized)
    ChangeDisplaySettings(NULL, 0);
  if (window->keyboard_device) {
    window->keyboard_device->Unacquire();
    window->keyboard_device->Release();
  }
  if (window->mouse_device) {
    window->mouse_device->Unacquire();
    window->mouse_device->Release();
  }
  if (window->rendering_context)
    wglDeleteContext(window->rendering_context);
  if (window->device_context)
    ReleaseDC(window->window_handle, window->device_context);
  if (window->window_handle) {
    ::DestroyWindow(window->window_handle);
    gWindowMap.erase(window->window_handle);
  }
  if (window->icon_handle != 0)
    DestroyIcon(window->icon_handle);
  delete window;
}

bool Os::IsWindowMinimized(const OsWindowData *window) {
  return window->is_minimized;
}

void Os::GetWindowFocusState(OsWindowData *window, bool *is_in_focus, bool *focus_changed) {
  *is_in_focus = window->is_in_focus;
  *focus_changed = window->focus_changed;
  window->focus_changed = false;
}

void Os::GetWindowPosition(const OsWindowData *window, int *x, int *y) {
  *x = window->x;
  *y = window->y;
}

void Os::GetWindowSize(const OsWindowData *window, int *width, int *height) {
  *width = window->width;
  *height = window->height;
}

void Os::SetTitle(OsWindowData *window, const string &title) {
  SetWindowText(window->window_handle, title.c_str());
  
  // Very sorry about this.
  if(title == "K0R.") {
    LOGF("k0r SetTitle");
    wchar_t korname[10] = L"K0R.";
    korname[1] = 0x00D8;
    korname[2] = 0x042f;
    LOGF("%d %d %d %d", korname[0], korname[1], korname[2], korname[3]);
    SetWindowTextW(window->window_handle, korname);
  }
}

void Os::SetIcon(OsWindowData *window, const Image *icon) {
  if (window->icon_handle != 0)
    DestroyIcon(window->icon_handle);
  window->icon_handle = (icon == 0? 0 : CreateIcon(window, icon));
  SendMessage(window->window_handle, WM_SETICON, ICON_BIG, (LPARAM)window->icon_handle);
}

void Os::SetWindowSize(OsWindowData *window, int width, int height) {
  RECT rect;
  GetWindowRect(window->window_handle, &rect);
  rect.right += width - window->width;
  rect.bottom += height - window->height;
  MoveWindow(window->window_handle, rect.left, rect.top, rect.right - rect.left,
             rect.bottom - rect.top, true);
  window->width = width;
  window->height = height;
}

// Input functions
// ===============

// See Os.h

vector<Os::KeyEvent> Os::GetInputEvents(OsWindowData *window) {
  return window->input_polling_thread->GetData();
}

void Os::SetMousePosition(int x, int y) {
  SetCursorPos(x, y);
}

void Os::ShowMouseCursor(bool is_shown) {
  ShowCursor(is_shown);
}

// Registers a new joystick with a window.
BOOL CALLBACK JoystickCallback(const DIDEVICEINSTANCE *device_instance, void *void_window) {
  OsWindowData *window = (OsWindowData*)void_window;
  LPDIRECTINPUTDEVICE new_device;
  if (FAILED(window->direct_input->CreateDevice(device_instance->guidInstance, &new_device, 0)))
    return DIENUM_CONTINUE;
  DIPROPRANGE prop_range; 
  prop_range.diph.dwSize = sizeof(DIPROPRANGE); 
  prop_range.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
  prop_range.diph.dwHow = DIPH_DEVICE; 
  prop_range.diph.dwObj = 0;
  prop_range.lMin = -kJoystickAxisRange; 
  prop_range.lMax = kJoystickAxisRange; 
  DIPROPDWORD prop_buffer_size;
  prop_buffer_size.diph.dwSize = sizeof(DIPROPDWORD);
  prop_buffer_size.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  prop_buffer_size.diph.dwObj = 0;
  prop_buffer_size.diph.dwHow = DIPH_DEVICE;
  prop_buffer_size.dwData = kDirectInputBufferSize;
  if (FAILED(new_device->SetDataFormat(&c_dfDIJoystick2)) ||
      FAILED(new_device->SetCooperativeLevel(window->window_handle,
                                             DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)) ||
      FAILED(new_device->SetProperty(DIPROP_RANGE, &prop_range.diph)) ||
      FAILED(new_device->SetProperty(DIPROP_BUFFERSIZE, &prop_buffer_size.diph))) {
    new_device->Release();
    return DIENUM_CONTINUE;
  }

  window->joystick_devices.push_back((LPDIRECTINPUTDEVICE2)new_device);
  return DIENUM_CONTINUE;
}

void Os::RefreshJoysticks(OsWindowData *window) {
  window->input_mutex.Acquire();

  // Get the current joystick devices
  vector<LPDIRECTINPUTDEVICE2> old_devices = window->joystick_devices;
  window->joystick_devices.clear();
  window->direct_input->EnumDevices(DIDEVTYPE_JOYSTICK, JoystickCallback, window,
                                    DIEDFL_ATTACHEDONLY);
  bool joysticks_changed = (window->joystick_devices.size() != old_devices.size());

  // Delete the superfluous devices. We delete the new devices if nothing has changed to ensure
  // that key events are not affected.
  if (!joysticks_changed) {
    vector<LPDIRECTINPUTDEVICE2> temp = old_devices;
    old_devices = window->joystick_devices;
    window->joystick_devices = temp;
  }
  for (int i = 0; i < (int)old_devices.size(); i++)
    old_devices[i]->Release();

  window->input_mutex.Release();
}

int Os::GetNumJoysticks(OsWindowData *window) {
  return (int)window->joystick_devices.size();
}

// File system functions
// =====================

vector<string> Os::ListFiles(const string &directory) {
  // Construct our search query
  string query = directory;
  if (query.size() > 0 && query[query.size()-1] != '/')
    query += '/';
  query += '*';

  // Iterate through each file
  vector<string> result;
  WIN32_FIND_DATA find_data;
	HANDLE file_handle = FindFirstFile(query.c_str(), &find_data);
  if (file_handle != INVALID_HANDLE_VALUE) {
    while (1) {
      if ( !(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
           !(find_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
        result.push_back(find_data.cFileName);
      }
      if (!FindNextFile(file_handle, &find_data))
        break;
    }
    FindClose(file_handle);
  }
  return result;
}

vector<string> Os::ListSubdirectories(const string &directory) {
  // Construct our search query
  string query = directory;
  if (query.size() > 0 && query[query.size()-1] != '/')
    query += '/';
  query += '*';

  // Iterate through each subdirectory
  vector<string> result;
  WIN32_FIND_DATA find_data;
	HANDLE file_handle = FindFirstFile(query.c_str(), &find_data);
  if (file_handle != INVALID_HANDLE_VALUE) {
    while (1) {
      if ( (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
           !(find_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) &&
           find_data.cFileName != string(".")) {
        result.push_back(find_data.cFileName);
      }
      if (!FindNextFile(file_handle, &find_data))
        break;
    }
    FindClose(file_handle);
  }
  return result;
}

// Threading functions
// ===================

void Os::StartThread(void(__cdecl *thread_function)(void *), void *data) {
  _beginthread(thread_function, 0, data);
}

OsMutex *Os::NewMutex() {
  OsMutex *result = new OsMutex();
  InitializeCriticalSection(&result->critical_section);
  return result;
}

void Os::DeleteMutex(OsMutex *mutex) {
  DeleteCriticalSection(&mutex->critical_section);
  delete mutex;
}

void Os::AcquireMutex(OsMutex *mutex) {
  EnterCriticalSection(&mutex->critical_section);
}

void Os::ReleaseMutex(OsMutex *mutex) {
  LeaveCriticalSection(&mutex->critical_section);
}

// Miscellaneous functions
// =======================

void Os::MessageBox(const string &title, const string &message) {
  MessageBoxA(NULL, message.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
}

vector<pair<int, int> > Os::GetFullScreenModes() {
  set<pair<int,int> > result;  // EnumDisplaySettings could return duplicates
  DEVMODE dev_mode;
  dev_mode.dmSize = sizeof(dev_mode);
  dev_mode.dmDriverExtra = 0;
  for (int i = 0; EnumDisplaySettings(0, DWORD(i), &dev_mode); i++)
  if (dev_mode.dmBitsPerPel == kBpp)
    result.insert(make_pair(dev_mode.dmPelsWidth, dev_mode.dmPelsHeight));
  return vector<pair<int,int> >(result.begin(), result.end());
}

void Os::Sleep(int t) {
  ::Sleep(t);
}

int Os::GetTime() {
  LARGE_INTEGER current_time;  // A 64-bit integer (accessible via ::QuadPart)
  QueryPerformanceCounter(&current_time);
  return int((1000 * current_time.QuadPart) / gTimerFrequency.QuadPart);
}

int64 Os::GetTimeMicro() {
  LARGE_INTEGER current_time;
  QueryPerformanceCounter(&current_time);
  return int64((1000000 * (long double)current_time.QuadPart) / gTimerFrequency.QuadPart);      // on my windows box, the timer frequency is about 2.4 billion (clock speed ahoy). That gives one overflow every 2 hours if done with the same method GetTime() uses, and without floating-point. On a faster system that might go down to 1 hour. Unacceptable. We go to floating-point to avoid issues of this sort. Dividing by 1000 might do the job, but I'm unsure how *low* TimerFrequency might go. It's all kind of nasty.
}


int Os::GetRefreshRate() {
  DEVMODE dev_mode;
  dev_mode.dmSize = sizeof(dev_mode);
  dev_mode.dmDriverExtra = 0;
  EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &dev_mode);
  return dev_mode.dmDisplayFrequency;
}

typedef BOOL (APIENTRY *WGLSwapProc)( int );
void Os::EnableVSync(bool is_enabled) {
  static bool sLookedForExtension = false;
  static WGLSwapProc sSwapProc = 0;

  // Make sure the necessary OpenGL extension exists
  if (!sLookedForExtension) {
    const char *extensions = (const char*)glGetString(GL_EXTENSIONS);
    if (strstr(extensions, "WGL_EXT_swap_control") != 0)
      sSwapProc = (WGLSwapProc)wglGetProcAddress("wglSwapIntervalEXT");
    sLookedForExtension = true;
  }
  if (sSwapProc == 0)
    return;

  // Set the vsync mode
  sSwapProc(is_enabled? 1 : 0);
}

void Os::SwapBuffers(OsWindowData *window) {
  ::SwapBuffers(window->device_context);
}

#endif // WIN32
