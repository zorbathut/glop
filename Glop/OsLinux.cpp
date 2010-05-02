#ifdef LINUX

#include "Os.h"
#include <vector>
#include <string>
#include <set>
#include <map>
#include <algorithm>
#include <cstdio>

#include <sys/time.h>

#include <X11/Xlib.h>
#include <GL/glx.h>

using namespace std;

Display *display = NULL;
int screen = 0;
XIM xim = NULL;



struct OsWindowData {
  OsWindowData() { window = NULL; }
  ~OsWindowData() {
    glXDestroyContext(display, context);
    XDestroyIC(inputcontext);
    XDestroyWindow(display, window);
  }
  
  Window window;
  GLXContext context;
  XIC inputcontext;
};


void Os::Init() {
  display = XOpenDisplay(NULL);
  ASSERT(display);
  
  screen = DefaultScreen(display);
  
  xim = XOpenIM(display, NULL, NULL, NULL);
  ASSERT(xim);
}
void Os::ShutDown() {
  XCloseIM(xim);
  XCloseDisplay(display);
}

vector<Os::KeyEvent> events;

Bool EventTester(Display *display, XEvent *event, XPointer arg) {
  return true; // hurrr
}
void Os::Think() { } // we don't actually do anything here
void Os::WindowThink(OsWindowData* data) {
  XEvent event;
  while(XCheckIfEvent(display, &event, &EventTester, NULL)) {
    LOGF("Event incoming!\n");
  }
}


OsWindowData* Os::CreateWindow(const string& title, int x, int y, int width, int height, bool full_screen, short stencil_bits, const Image* icon, bool is_resizable) {
  OsWindowData *nw = new OsWindowData();
  
  // this is bad
  if(x == -1) x = 100;
  if(y == -1) y = 100;
  
  // I think there's a way to specify more of this but I'll be damned if I can find it
  XVisualInfo vinfo_template;
  vinfo_template.screen = screen;
  int vinfo_ct;
  XVisualInfo *visualinfo = XGetVisualInfo(display, VisualScreenMask, &vinfo_template, &vinfo_ct);
  ASSERT(visualinfo);
  ASSERT(vinfo_ct);
  
  XVisualInfo vinfo;
  bool found = false;
  for(int i = 0; i < vinfo_ct; i++) {
    int use_gl, rgba, doublebuffer, red_size, green_size, blue_size, alpha_size;
    glXGetConfig(display, &visualinfo[i], GLX_USE_GL, &use_gl);
    glXGetConfig(display, &visualinfo[i], GLX_RGBA, &rgba);
    glXGetConfig(display, &visualinfo[i], GLX_DOUBLEBUFFER, &doublebuffer);
    glXGetConfig(display, &visualinfo[i], GLX_RED_SIZE, &red_size);
    glXGetConfig(display, &visualinfo[i], GLX_GREEN_SIZE, &green_size);
    glXGetConfig(display, &visualinfo[i], GLX_BLUE_SIZE, &blue_size);
    glXGetConfig(display, &visualinfo[i], GLX_ALPHA_SIZE, &alpha_size);
    //LOGF("%d %d %d %d %d %d %d", use_gl, rgba, doublebuffer, red_size, green_size, blue_size, alpha_size);
    
    
    if(use_gl && rgba && doublebuffer && red_size == 8 && green_size == 8 && blue_size == 8 && alpha_size == 8) {
      LOGF("Found something!");
      found = true;
      vinfo = visualinfo[i]; // I'm just going to hope the rest of the values are appropriate, because, really, who knows
      break;
    }
  }
  
  ASSERT(found);
  
  nw->context = glXCreateContext(display, &vinfo, NULL, True);
  ASSERT(nw->context);
  
  nw->window = XCreateWindow(display, RootWindow(display, screen), x, y, width, height, 0, vinfo.depth, InputOutput, vinfo.visual, 0, NULL); // I don't know if I need anything further here
  
  SetTitle(nw, title);
  
  // I think in here is where we're meant to set window styles and stuff
  
  nw->inputcontext = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, nw->window, XNFocusWindow, nw->window, NULL);
  ASSERT(nw->inputcontext);
  
  XFree(visualinfo);
  
  XMapWindow(display, nw->window);
  
  SetCurrentContext(nw);
  
  return nw;
}

void Os::SetCurrentContext(OsWindowData* data) {
  glXMakeCurrent(display, data->window, data->context);
}


// Destroys a window that is completely or partially created.
void Os::DestroyWindow(OsWindowData* data) {
  delete data;
}

bool Os::IsWindowMinimized(const OsWindowData* data) {
  return false;
}

void Os::GetWindowFocusState(OsWindowData* data, bool* is_in_focus, bool* focus_changed) {
  *is_in_focus = true;
  *focus_changed = false;
}

void Os::GetWindowPosition(const OsWindowData* data, int* x, int* y) {
  XWindowAttributes attrs;
  XGetWindowAttributes(display, data->window, &attrs);
  *x = attrs.x;
  *y = attrs.y;
}

void Os::GetWindowSize(const OsWindowData* data, int* width, int* height) {
  XWindowAttributes attrs;
  XGetWindowAttributes(display, data->window, &attrs);
  *width  = attrs.width;
  *height = attrs.height;
}

void Os::SetTitle(OsWindowData* data, const string& title) {
  XStoreName(display, data->window, title.c_str());
}

void Os::SetIcon(OsWindowData *window, const Image *icon) {
  fprintf(stderr, "Os::SetIcon(%p, %p)\n", window, icon); // TBI
}

void Os::SetWindowSize(OsWindowData *window, int width, int height) {
  fprintf(stderr, "Os::SetWindowSize(%p, %d, %d)\n", window, width, height); // TBI
}


// Input functions
// ===============

// See Os.h

vector<Os::KeyEvent> Os::GetInputEvents(OsWindowData *window) {
  vector<Os::KeyEvent> ret; // weeeeeeeeeeee
  ret.swap(events);
  
  Window root, child;
  int x, y, winx, winy;
  unsigned int mask;
  XQueryPointer(display, window->window, &root, &child, &x, &y, &winx, &winy, &mask);
  
  ret.push_back(Os::KeyEvent(GetTime(), x, y, mask & (1 << 4), mask & LockMask));
  
  return ret;
}

void Os::SetMousePosition(int x, int y) { // TBI
}

void Os::ShowMouseCursor(bool is_shown) { // TBI
}

void Os::RefreshJoysticks(OsWindowData *window) { // TBI
}

int Os::GetNumJoysticks(OsWindowData *window) { // TBI
  return 0;
}

// Threading functions
// ===================

#include <pthread.h>

void Os::StartThread(void(*thread_function)(void*), void* data) {
  pthread_t thread;
  if (pthread_create(&thread, NULL, (void*(*)(void*))thread_function, data) != 0) {
    printf("Error forking thread\n");
  }
}

struct OsMutex {
  pthread_mutex_t mutex;
};

OsMutex* Os::NewMutex() {
  OsMutex* mutex = new OsMutex;
  pthread_mutex_init(&mutex->mutex, NULL);
  return mutex;
}

void Os::DeleteMutex(OsMutex* mutex) {
  pthread_mutex_destroy(&mutex->mutex);
  delete mutex;
}

void Os::AcquireMutex(OsMutex* mutex) {
  pthread_mutex_lock(&mutex->mutex);
}

void Os::ReleaseMutex(OsMutex* mutex) {
  pthread_mutex_unlock(&mutex->mutex);
}

// Miscellaneous functions
// =======================

void Os::MessageBox(const string& title, const string& message) { // TBI
  fprintf(stderr,"MessageBox [%s]: [%s]\n", title.c_str(), message.c_str());
}

vector<pair<int, int> > Os::GetFullScreenModes() { // TBI
  return vector<pair<int, int> >(1,pair<int, int>(640,480));
}


void Os::Sleep(int t) {
  usleep(t*1000);
}

int Os::GetTime() {
  return GetTimeMicro() / 1000;
}
long long Os::GetTimeMicro() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}


void Os::SwapBuffers(OsWindowData* data) {
  glXSwapBuffers(display, data->window);
}

int Os::GetRefreshRate() { // TBI
  //fprintf(stderr,"OS::GetRefreshRate()\n");
  return 60;
}

void Os::EnableVSync(bool is_enable) { // TBI
  fprintf(stderr,"OS::EnableVSync(%d)\n", is_enable);
}


vector<string> Os::ListFiles(const string &directory) { // TBI
  fprintf(stderr,"OS::ListFiles(%s)\n", directory.c_str());
  return vector<string>();
}

vector<string> Os::ListSubdirectories(const string &directory) { // TBI
  fprintf(stderr,"OS::ListSubdirectories(%s)\n", directory.c_str());
  return vector<string>();
}


#endif // LINUX

