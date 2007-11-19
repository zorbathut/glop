#ifdef MACOSX

#include "Os.h"
#include <vector>
#include <string>
#include <set>
using namespace std;

#include <Carbon/Carbon.h>
#include <OpenGL/gl.h>
#include <AGL/agl.h>

const int kEventClassGlop = 777;
const int kEventGlopBreak = 0;

static set<OsWindowData*> all_windows;

struct OsWindowData {
  OsWindowData() {
    window = NULL;
    agl_context = NULL;
    pixel_format = NULL;
  }
  ~OsWindowData() {
    if (window != NULL) {
      DisposeWindow(window);
    }
    if (agl_context != NULL) {
      aglDestroyContext(agl_context);
    }
    if (pixel_format != NULL) {
      aglDestroyPixelFormat(pixel_format);
    }
  }
  WindowRef window;
  AGLContext agl_context;
  AGLPixelFormat pixel_format;  // We might not even need to keep this around at all
  Rect bounds;
  string title;
};

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
  }
  return result;
}

void Os::Init() {
  EventHandlerUPP handler_upp = NewEventHandlerUPP(GlopEventHandler);
  EventTypeSpec event_types[2];
  event_types[0].eventClass = kEventClassGlop;
  event_types[0].eventKind  = kEventGlopBreak;
  event_types[1].eventClass = kEventClassApplication;
  event_types[1].eventKind  = kEventAppQuit;
  InstallApplicationEventHandler(handler_upp, 2, event_types, NULL, NULL);
}

void Os::Think() {
  EventRef terminator;
  CreateEvent(NULL, kEventClassGlop, kEventGlopBreak, 0, kEventAttributeNone, &terminator);
  PostEventToQueue(GetMainEventQueue(), terminator, kEventPriorityLow);
  ok_to_exit = true;
  RunApplicationEventLoop();
  ReleaseEvent(terminator);
  set<OsWindowData*>::iterator it;
  for (it = all_windows.begin(); it != all_windows.end(); it++) {
    WindowThink(*it);
  }
  usleep(10);
}

void Os::WindowThink(OsWindowData* data) {
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

void CreateAGLContext(OsWindowData* data) {
  OSStatus err = noErr;
  GLint attributes[] = {
    AGL_RGBA,
    AGL_DOUBLEBUFFER,
    AGL_DEPTH_SIZE, 24,
    AGL_NONE
  };

  // TODO(jwills): Find out why on earth calling aglChoosePixelFormat starts another thread!
  data->pixel_format = aglChoosePixelFormat(NULL, 0, attributes);
  err = aglReportError();
  if (data->pixel_format) {
    data->agl_context = aglCreateContext(data->pixel_format, NULL);
    err = aglReportError();
  }
  aglSetHIViewRef(data->agl_context, HIViewGetRoot(data->window));
//  glViewport(0, 0, data->bounds.right - data->bounds.left, data->bounds.bottom - data->bounds.top);
//  if (!aglSetDrawable(data->agl_context, GetWindowPort(data->window)))
//    err = aglReportError();
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
  SetRect(&data->bounds, x, y, x + width, y + height);
  data->title = title;
//  OSStatus result =
      CreateNewWindow(
          kDocumentWindowClass,
              kWindowStandardDocumentAttributes |
              kWindowStandardHandlerAttribute |
              kWindowAsyncDragAttribute |
              kWindowLiveResizeAttribute,
		  &data->bounds,
		  &data->window);
//kEventWindowResizeCompleted
  EventTypeSpec event_types[1];
  event_types[0].eventClass = kEventClassWindow;
  event_types[0].eventKind  = kEventWindowResizeCompleted;
  EventHandlerUPP handler_upp = NewEventHandlerUPP(GlopWindowHandler);
  InstallWindowEventHandler(data->window, handler_upp, 1, event_types, data, NULL);
                               
                               
                               
  CFStringRef cf_title;
  cf_title = CFStringCreateWithCString(NULL, data->title.c_str(), kCFStringEncodingASCII);  
  SetWindowTitleWithCFString(data->window, cf_title);
  CFRelease(cf_title);
  CreateAGLContext(data);
  SetCurrentContext(data);
  SelectWindow(data->window);
  ShowWindow(data->window);
  all_windows.insert(data);
  return data;
}

void Os::SetCurrentContext(OsWindowData* data) {
  if (data->agl_context == NULL || aglGetCurrentContext() == data->agl_context)
    return;
  if (!aglSetCurrentContext(data->agl_context))
    aglReportError();
  if (!aglUpdateContext(data->agl_context))
    aglReportError();
}


// Destroys a window that is completely or partially created.
void Os::DestroyWindow(OsWindowData* window) {
  all_windows.erase(window);
  delete window;
}

bool Os::IsWindowMinimized(const OsWindowData *window) {
  return true;
}

void Os::GetWindowFocusState(OsWindowData *window, bool *is_in_focus, bool *focus_changed) {
}

void Os::GetWindowPosition(const OsWindowData *window, int *x, int *y) {
}

void Os::GetWindowSize(const OsWindowData *window, int *width, int *height) {
}

void Os::SetTitle(OsWindowData *window, const string &title) {
}

void Os::SetIcon(OsWindowData *window, const Image *icon) {
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

vector<pair<int, int> > Os::GetFullScreenModes() {
  return vector<pair<int,int> >();
}

void Os::Sleep(int t) {
}

int Os::GetTime() {
	return 0;
}

void Os::SwapBuffers(OsWindowData *window) {

}

#endif // MACOSX