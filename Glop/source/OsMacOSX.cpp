#ifdef MACOSX

#include "Os.h"
#include <vector>
#include <string>
#include <set>
using namespace std;

#include <Carbon/Carbon.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <AGL/agl.h>
#include <ApplicationServices/ApplicationServices.h>

const int kEventClassGlop = 'Glop';
const int kEventGlopBreak = 0;
const int kEventGlopToggleFullScreen = 'Flsc';

static set<OsWindowData*> all_windows;

struct OsWindowData {
  OsWindowData() :
      window(NULL),
      agl_context(NULL),
      was_active(false) {
  }
  ~OsWindowData() {
    if (window != NULL) {
      DisposeWindow(window);
    }
    if (agl_context != NULL) {
      aglDestroyContext(agl_context);
    }
  }
  WindowRef window;
  AGLContext agl_context;
  Rect bounds;
  Rect full_screen_dimensions;
  string title;
  bool full_screen;
  bool was_active;
};

void GlopToggleFullScreen();

// HACK!
static bool ok_to_exit;

OSStatus GlopEventHandler(EventHandlerCallRef next_handler, EventRef the_event, void* user_data) {
  OSStatus result = eventNotHandledErr;
  int event_class = GetEventClass(the_event);
  int event_kind = GetEventKind(the_event);
  if (event_class == kEventClassGlop && event_kind == kEventGlopBreak) {
    ok_to_exit = false;
    QuitApplicationEventLoop();
    result = noErr;
  }
  if (event_class == kEventClassApplication && event_kind == kEventAppQuit) {
    if (ok_to_exit)
      exit(0);
  }
  if (event_class == kEventClassCommand) {
    if (event_kind == kEventProcessCommand) {
      HICommand command;
    	GetEventParameter(
    	    the_event,
    	    kEventParamDirectObject,
    	    kEventParamHICommand,
    	    NULL,
    	    sizeof(command),
    	    NULL,
          &command);
      if (command.commandID == kEventGlopToggleFullScreen) {
        printf("Toggle\n");
        GlopToggleFullScreen();
      }
    }
  }
  return result;
}

OSStatus GlopWindowHandler(EventHandlerCallRef next_handler, EventRef the_event, void* user_data) {
  OSStatus result = eventNotHandledErr;
  int event_class = GetEventClass(the_event);
  int event_kind = GetEventKind(the_event);
  OsWindowData* data = (OsWindowData*)user_data;
  if (event_class == kEventClassWindow) {
    if (event_kind == kEventWindowResizeCompleted) {
      GetWindowPortBounds(data->window, &data->bounds);
      Os::SetCurrentContext(data);
			glViewport(
			    0,
			    0,
			    data->bounds.right - data->bounds.left,
			    data->bounds.bottom - data->bounds.top);
        printf("%s: (%d,%d)\n",data->title.c_str(), data->bounds.right-data->bounds.left, data->bounds.bottom - data->bounds.top);
    }
    if (event_kind == kEventWindowClosed) {
      printf("Destroying window %s\n", data->title.c_str());
      if (!data->full_screen) {
        Os::DestroyWindow(data);
      }
    }
  }
  return result;
}

static UnsignedWide glop_start_time;
void Os::Init() {
  Microseconds(&glop_start_time);
  EventHandlerUPP handler_upp = NewEventHandlerUPP(GlopEventHandler);
  EventTypeSpec event_types[3];
  event_types[0].eventClass = kEventClassGlop;
  event_types[0].eventKind  = kEventGlopBreak;
  event_types[1].eventClass = kEventClassApplication;
  event_types[1].eventKind  = kEventAppQuit;
  event_types[2].eventClass = kEventClassCommand;
  event_types[2].eventKind  = kEventProcessCommand;
  InstallApplicationEventHandler(handler_upp, 3, event_types, NULL, NULL);

  OSStatus err;
  IBNibRef nib_ref = NULL;
  CFBundleRef bundle;
  bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.thunderproductions.glopframework"));
  err = CreateNibReferenceWithCFBundle(bundle, CFSTR("main"), &nib_ref);
  err = SetMenuBarFromNib(nib_ref, CFSTR("MainMenu"));
  DisposeNibReference(nib_ref);
}

void Os::Terminate() {
  // Booya! :-)
}

void Os::Think() {
  // Static since we don't want to create and free this event every time we think
  static EventRef terminator = NULL;
  if (terminator == NULL) {
    CreateEvent(NULL, kEventClassGlop, kEventGlopBreak, 0, kEventAttributeNone, &terminator);
  }
  PostEventToQueue(GetMainEventQueue(), terminator, kEventPriorityLow);
  ok_to_exit = true;
  RunApplicationEventLoop();
  set<OsWindowData*>::iterator it;
  for (it = all_windows.begin(); it != all_windows.end(); it++) {
    WindowThink(*it);
  }
}

void Os::WindowThink(OsWindowData* data) {
//  CGLFlushDrawable(CGLGetCurrentContext());
  if (data->agl_context == NULL) {
    return;
  }
  aglSwapBuffers(data->agl_context);
}

OSStatus aglReportError(void) {
	GLenum err = aglGetError();
	if (AGL_NO_ERROR != err) {
		char errStr[256];
		printf(errStr, "AGL: %s",(char *) aglErrorString(err));
	}
	// ensure we are returning an OSStatus noErr if no error condition
	if (err == AGL_NO_ERROR)
		return noErr;
	else
		return (OSStatus) err;
}

// maybe this should just return the context
void GlopCreateAGLContext(OsWindowData* data) {
  OSStatus err = noErr;
  GLint attributes[] = {
    AGL_RGBA,
    AGL_DOUBLEBUFFER,
    AGL_DEPTH_SIZE, 32,
    (data->full_screen ? AGL_FULLSCREEN : AGL_NONE),
    AGL_NONE
  };

  AGLPixelFormat pixel_format;
  // TODO(jwills): Find out why on earth calling aglChoosePixelFormat starts another thread!
  pixel_format = aglChoosePixelFormat(NULL, 0, attributes);
  err = aglReportError();
  if (pixel_format) {
    data->agl_context = aglCreateContext(pixel_format, NULL);
    err = aglReportError();
    aglDestroyPixelFormat(pixel_format);
  }
  aglSetHIViewRef(data->agl_context, HIViewGetRoot(data->window));
  if (data->full_screen) {
    aglSetFullScreen(data->agl_context, 0, 0, 0, 0);
  }
}

void GlopEnterFullScreen(OsWindowData* data) {
  GlopCreateAGLContext(data);
  printf("Entering fullscreen with dimensions %d,%d\n", data->full_screen_dimensions.right,data->full_screen_dimensions.bottom);
  aglSetFullScreen(
      data->agl_context,
      data->full_screen_dimensions.right,
      data->full_screen_dimensions.bottom,
      0,
      0);
}

void GlopOpenWindow(OsWindowData* data) {
//  OSStatus result =
  printf("%d %d %d %d\n", data->bounds.left, data->bounds.right, data->bounds.top, data->bounds.bottom);
      CreateNewWindow(
          kDocumentWindowClass,
              kWindowCollapseBoxAttribute |
              kWindowResizableAttribute |
              kWindowStandardHandlerAttribute |
              kWindowAsyncDragAttribute |
              kWindowLiveResizeAttribute,
		  &data->bounds,
		  &data->window);

  EventTypeSpec event_types[2];
  event_types[0].eventClass = kEventClassWindow;
  event_types[0].eventKind  = kEventWindowResizeCompleted;
  event_types[1].eventClass = kEventClassWindow;
  event_types[1].eventKind  = kEventWindowClosed;
  EventHandlerUPP handler_upp = NewEventHandlerUPP(GlopWindowHandler);
  InstallWindowEventHandler(data->window, handler_upp, 2, event_types, data, NULL);
  GlopCreateAGLContext(data);

  Os::SetTitle(data, data->title);
  Os::SetCurrentContext(data);
  SelectWindow(data->window);
  ShowWindow(data->window);
}

void GlopToggleFullScreen() {
  // First find the window with the focus
  printf("GlopToggleFullScreen()\n");
  OsWindowData* data = NULL;
  OsWindowData* full_screen_data = NULL;
  set<OsWindowData*>::iterator it;
  for (it = all_windows.begin(); it != all_windows.end(); it++) {
    if (IsWindowActive((*it)->window) || (*it)->full_screen) {
      data = *it;
      if (data->full_screen) {
        full_screen_data = *it;
      }
    }
  }
  if (data == NULL) {
    // We can't go into fullscreen if we don't know what window to apply this to
    return;
  }
  if (full_screen_data != NULL) {
    data = full_screen_data;
  }
  printf("GlopToggleFullScreen(%s)\n", data->title.c_str());
//  aglSetDrawable(NULL, NULL);
  data->full_screen = !data->full_screen;
  if (!data->full_screen) {
    printf("Currently fullscreen, making windowed\n");
    aglDestroyContext(data->agl_context);
    data->agl_context = NULL;
    GlopOpenWindow(data);
  } else {
    printf("Currently windowed, making fullscreen\n");
    DisposeWindow(data->window);
    aglDestroyContext(data->agl_context);
    GlopEnterFullScreen(data);
  }
  if (data->full_screen)
    printf("Now fullscreened\n");
  else
    printf("Now windowed\n");
}

OsWindowData* Os::CreateWindow(
    const string& title,
    int x,
    int y,
    int width,
    int height,
    bool full_screen,
    short stencil_bits,
    const Image* icon,
    bool is_resizable) {
  OsWindowData* data = new OsWindowData();
  data->full_screen = full_screen;
  data->title = title;
  if (full_screen) {
    SetRect(&data->bounds, 35, 35, width/2, height/2);  // TODO(jwills): Decide what to really do here
    SetRect(&data->full_screen_dimensions, 0, 0, width, height);
    GlopEnterFullScreen(data);
  } else {
    SetRect(&data->bounds, x, y, x + width, y + height);
    SetRect(&data->full_screen_dimensions, 0, 0, 1600, 1050);
    GlopOpenWindow(data);
  }
  all_windows.insert(data);
  return data;
}

void Os::SetCurrentContext(OsWindowData* data) {
  if (data->agl_context == NULL || aglGetCurrentContext() == data->agl_context) {
    if (data->agl_context == NULL)
      printf("No agl context, can't set context.\n");
    return;
  }
  if (!aglSetCurrentContext(data->agl_context))
    aglReportError();
  if (!aglUpdateContext(data->agl_context))
    aglReportError();
}


// Destroys a window that is completely or partially created.
void Os::DestroyWindow(OsWindowData* data) {
  all_windows.erase(data);
  delete data;
}

bool Os::IsWindowMinimized(const OsWindowData* data) {
  return IsWindowCollapsed(data->window);
}

void Os::GetWindowFocusState(OsWindowData* data, bool* is_in_focus, bool* focus_changed) {
  bool active = IsWindowActive(data->window);
  *is_in_focus = active;
  *focus_changed = active != data->was_active;
  data->was_active = active;
}

void Os::GetWindowPosition(const OsWindowData* data, int* x, int* y) {
  *x = data->bounds.left;
  *y = data->bounds.top;
}

void Os::GetWindowSize(const OsWindowData* data, int* width, int* height) {
  *width  = data->bounds.left - data->bounds.right;
  *height = data->bounds.top - data->bounds.bottom;
}

void Os::SetTitle(OsWindowData* data, const string& title) {
  CFStringRef cf_title;
  data->title = title;
  cf_title = CFStringCreateWithCString(NULL, data->title.c_str(), kCFStringEncodingASCII);  
  SetWindowTitleWithCFString(data->window, cf_title);
  CFRelease(cf_title);
}

void Os::SetIcon(OsWindowData *window, const Image *icon) {
  // TODO(jwills): Figure out exactly how this should work considering that it needs to run on PCs
  // and on macs
}

// Input functions
// ===============

// See Os.h

vector<Os::KeyEvent> Os::PollInput(OsWindowData *window, bool *is_num_lock_set,
                                   bool *is_caps_lock_set, int *cursor_x, int *cursor_y,
                                   int *mouse_dx, int *mouse_dy) {
  return vector<Os::KeyEvent>();
}

void Os::SetMousePosition(int x, int y) {
}

void Os::ShowMouseCursor(bool is_shown) {
}

void Os::RefreshJoysticks(OsWindowData *window) {
}

int Os::GetNumJoysticks(OsWindowData *window) {
	return 0;
}

// Threading functions
// ===================

void Os::StartThread(void(*thread_function)(void *), void *data) {
}

OsMutex *Os::NewMutex() {
	return NULL;
}

void Os::DeleteMutex(OsMutex *mutex) {
}

void Os::AcquireMutex(OsMutex *mutex) {
}

void Os::ReleaseMutex(OsMutex *mutex) {
}

// Miscellaneous functions
// =======================

void Os::DisplayMessage(const string &title, const string &message) {
}

// TODO(jwills): Currently we only deal with the main display, decide whether or not we should be
// able to display on other or multiple displays.
vector<pair<int, int> > Os::GetFullScreenModes() {
  CFArrayRef modes = CGDisplayAvailableModes(kCGDirectMainDisplay);
  set<pair<int,int> > modes_set;
  for (int i = 0; i < CFArrayGetCount(modes); i++) {
    CFDictionaryRef attributes = (CFDictionaryRef)CFArrayGetValueAtIndex(modes, i);
    CFNumberRef width_number  = (CFNumberRef)CFDictionaryGetValue(attributes, kCGDisplayWidth);
    CFNumberRef height_number = (CFNumberRef)CFDictionaryGetValue(attributes, kCGDisplayHeight);
    int width;
    int height;
    CFNumberGetValue(width_number,  kCFNumberIntType, &width);
    CFNumberGetValue(height_number, kCFNumberIntType, &height);
    modes_set.insert(pair<int,int>(width, height));
  }
  return vector<pair<int,int> >(modes_set.begin(), modes_set.end());
}

void Os::Sleep(int t) {
  sleep(t);
}

int Os::GetTime() {
  UnsignedWide current_time;
  Microseconds(&current_time);
  unsigned long long start_time = (unsigned long long)glop_start_time.hi << 32 | glop_start_time.lo;
  return (((unsigned long long)current_time.hi << 32 | current_time.lo) - start_time) / 1000.0;
}

void Os::SwapBuffers(OsWindowData* window) {
}

#endif // MACOSX