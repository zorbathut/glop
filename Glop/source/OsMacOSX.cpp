#include "Os.h"
#include <vector>
using namespace std;

#include <iostream>
void Os::Think() {
	cout << "Foo!\n";
}

void Os::WindowThink(OsWindowData *window) {}
OsWindowData *Os::CreateWindow(const string &title, int x, int y, int width, int height,
                                    bool full_screen, short stencil_bits, const Image *icon,
                                    bool is_resizable)
{
	return NULL;
}

// Destroys a window that is completely or partially created.
void Os::DestroyWindow(OsWindowData *window) {
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
