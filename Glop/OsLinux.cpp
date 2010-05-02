#ifdef LINUX

#include "Os.h"
#include <vector>
#include <string>
#include <set>
#include <map>
#include <algorithm>
#include <cstdio>
using namespace std;

#include <GL/gl.h>


struct OsWindowData {
  OsWindowData() { fprintf(stderr,"OsWindowData()\n"); }
  ~OsWindowData() { fprintf(stderr, "~OsWindowData()\n"); }
};


void Os::Init() {
  fprintf(stderr,"Os::Init()\n");
}

void Os::ShutDown() {
  fprintf(stderr,"Os::ShutDown()\n");
}

void Os::Think() {
}

void Os::WindowThink(OsWindowData* data) {
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
  return new OsWindowData();
}

void Os::SetCurrentContext(OsWindowData* data) {
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
  *x = 0;
  *y = 0;
}

void Os::GetWindowSize(const OsWindowData* data, int* width, int* height) {
  fprintf(stderr, "Os::GetWindowSize(%p, %p, %p)\n", data, width, height);
  *width  = 320;
  *height = 200;
}

void Os::SetTitle(OsWindowData* data, const string& title) {
  fprintf(stderr, "Os::SetTitle(%p,%p)\n", data, title.c_str());
}

void Os::SetIcon(OsWindowData *window, const Image *icon) {
  fprintf(stderr, "Os::SetIcon(%p, %p)\n", window, icon);
}

void Os::SetWindowSize(OsWindowData *window, int width, int height) {
  fprintf(stderr, "Os::SetWindowSize(%p, %d, %d)\n", window, width, height);
}


// Input functions
// ===============

// See Os.h

vector<Os::KeyEvent> Os::GetInputEvents(OsWindowData *window) {
  vector<Os::KeyEvent> ret;
  return ret;
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

void Os::MessageBox(const string& title, const string& message) {
  fprintf(stderr,"MessageBox [%s]: [%s]\n", title.c_str(), message.c_str());
}

vector<pair<int, int> > Os::GetFullScreenModes() {
  return vector<pair<int, int> >(1,pair<int, int>(320,200));
}


void Os::Sleep(int t) {
  usleep(t*1000);
}

int Os::GetTime() {
  static int thetime = 0;
  return thetime++;
}
long long Os::GetTimeMicro() {
  static long long thetime = 0;
  return thetime++;
}


void Os::SwapBuffers(OsWindowData* data) {
  fprintf(stderr,"OS::SwapBuffers()\n");
}

int Os::GetRefreshRate() {
  fprintf(stderr,"OS::GetRefreshRate()\n");
  return 60;
}

void Os::EnableVSync(bool is_enable) {
  fprintf(stderr,"OS::EnableVSync(%d)\n", is_enable);
}


vector<string> Os::ListFiles(const string &directory) {
  fprintf(stderr,"OS::ListFiles(%s)\n", directory.c_str());
  return vector<string>();
}

vector<string> Os::ListSubdirectories(const string &directory) {
  fprintf(stderr,"OS::ListSubdirectories(%s)\n", directory.c_str());
  return vector<string>();
}


#endif // LINUX

