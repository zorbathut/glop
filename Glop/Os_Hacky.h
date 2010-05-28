// Platform-specific OS hacks that really shouldn't exist. Included until they can be dealt with in a better fashion.

#ifndef GLOP_OS_HACKY_H__
#define GLOP_OS_HACKY_H__

#ifdef IPHONE
// this is a horrible hack until I figure out how to build it into glop better
struct TouchInfo {
  bool active;
  float x, y;
};
struct TouchEvent {
  int type;
  int id;
  
  float x, y;
};
enum {EVENT_TOUCH, EVENT_MOVE, EVENT_RELEASE};

int os_touch_getCount();
bool os_touch_getActive(int id);
float os_touch_getX(int id);
float os_touch_getY(int id);

vector<TouchEvent> os_touch_getEvents();  // clears the event list in the process
#endif

#ifdef WIN32
// horrible hack to get more control over the windows icon - the whole icon thing needs to be redesigned
#include <windows.h>
HWND get_first_handle();
#endif

#ifdef LINUX
#include <X11/Xlib.h>
Display *get_x_display();
int get_x_screen();
Window get_x_window();
#endif

#endif
