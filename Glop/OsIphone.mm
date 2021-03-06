
#include "os.h"

#include <sys/time.h>

#import <UIKit/UIKit.h>
#import "OsIphone_EAGLView.h"

#include <map>
#include <vector>

using namespace std;

struct OsWindowData { };

#define SIGNAL_LAUNCHED 1
#define SIGNAL_RESIGN 2
#define SIGNAL_ACTIVE 3
#define SIGNAL_TERMINATE 4
#define SIGNAL_TIMER 5

static jmp_buf sig;
static jmp_buf rt;

// *********************************************
// *                     GlopAppDelegate
@interface GlopAppDelegate : NSObject <UIApplicationDelegate> {
}
@end

GlopAppDelegate *gad;

@implementation GlopAppDelegate
- (void) applicationDidFinishLaunching:(UIApplication *)application
{
  gad = self;
  printf("did finish launching, creating evil\n"); fflush(stdout);
  if(!setjmp(rt))
    longjmp(sig, SIGNAL_LAUNCHED);
}

- (void) applicationWillResignActive:(UIApplication *)application
{
  printf("Will Resign hit\n"); fflush(stdout);
  if(!setjmp(rt))
    longjmp(sig, SIGNAL_RESIGN);
}

- (void) applicationDidBecomeActive:(UIApplication *)application
{
  printf("Did Become Active hit!\n"); fflush(stdout);
  if(!setjmp(rt))
    longjmp(sig, SIGNAL_ACTIVE);
}

- (void)applicationWillTerminate:(UIApplication *)application
{
  printf("terminate hit\n"); fflush(stdout);
  if(!setjmp(rt))
    longjmp(sig, SIGNAL_TERMINATE);
}

- (void)timerYay
{
  //printf("timer hit\n"); fflush(stdout);
  if(!setjmp(rt))
    longjmp(sig, SIGNAL_TIMER);
}
@end


// *********************************************
// *                     EaterOfTouches
@interface EaterOfTouches : UIWindow {
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event;
@end

map<UITouch *, int> touch_ids;

vector<int> touch_ids_free;
int touch_ids_free_next = 0;

vector<TouchInfo> touches;
vector<TouchEvent> events;

int os_touch_getCount() { return touches.size(); }
bool os_touch_getActive(int id) { return touches[id].active; }
float os_touch_getX(int id) { return touches[id].x; }
float os_touch_getY(int id) { return touches[id].y; }

vector<TouchEvent> os_touch_getEvents() {
  vector<TouchEvent> eve;
  eve.swap(events);
  return eve;
}

@implementation EaterOfTouches
- (void)touchesBegan:(NSSet *)touches_set withEvent:(UIEvent *)event {
  NSArray *touch = [touches_set allObjects];
  for(int i = 0; i < [touch count]; i++) {
    UITouch *tt = [touch objectAtIndex:i];
    ASSERT(!touch_ids.count(tt));
    
    int nt;
    if(touch_ids_free.size()) {
      nt = touch_ids_free.back();
      touch_ids_free.pop_back();
    } else {
      nt = touch_ids_free_next++;
      touches.resize(touch_ids_free_next);
    }
    
    touch_ids[tt] = nt;
    
    TouchEvent te;
    te.type = EVENT_TOUCH;
    te.id = nt;
    te.x = [tt locationInView:self].x;
    te.y = [tt locationInView:self].y;
    events.push_back(te);
    
    touches[nt].x = te.x;
    touches[nt].y = te.y;
    touches[nt].active = true;
  }
}
- (void)touchesMoved:(NSSet *)touches_set withEvent:(UIEvent *)event {
  NSArray *touch = [touches_set allObjects];
  for(int i = 0; i < [touch count]; i++) {
    UITouch *tt = [touch objectAtIndex:i];
    ASSERT(touch_ids.count(tt));
    int nt = touch_ids[tt];
    
    TouchEvent te;
    te.type = EVENT_MOVE;
    te.id = nt;
    te.x = [tt locationInView:self].x;
    te.y = [tt locationInView:self].y;
    events.push_back(te);
    
    printf("%d %f/%f\n", nt, te.x, te.y); fflush(stdout);
    touches[nt].x = te.x;
    touches[nt].y = te.y;
  }
}
- (void)touchesEnded:(NSSet *)touches_set withEvent:(UIEvent *)event {
  NSArray *touch = [touches_set allObjects];
  for(int i = 0; i < [touch count]; i++) {
    UITouch *tt = [touch objectAtIndex:i];
    ASSERT(touch_ids.count(tt));
    int nt = touch_ids[tt];
    
    TouchEvent te;
    te.type = EVENT_RELEASE;
    te.id = nt;
    te.x = [tt locationInView:self].x;
    te.y = [tt locationInView:self].y;
    events.push_back(te);
    
    touch_ids.erase(tt);
    touch_ids_free.push_back(nt);
    touches[nt].active = false;
  }
}
- (void)touchesCancelled:(NSSet *)touches_set withEvent:(UIEvent *)event {
  NSArray *touch = [touches_set allObjects];
  for(int i = 0; i < [touch count]; i++) {
    UITouch *tt = [touch objectAtIndex:i];
    ASSERT(touch_ids.count(tt));
    int nt = touch_ids[tt];
    
    TouchEvent te;
    te.type = EVENT_RELEASE;  // good enough
    te.id = nt;
    te.x = [tt locationInView:self].x;
    te.y = [tt locationInView:self].y;
    events.push_back(te);
    
    touch_ids.erase(tt);
    touch_ids_free.push_back(nt);
    touches[nt].active = false;
  }
}
@end


NSAutoreleasePool * pool;
/*
void *appthread_func(void *) {
  printf("we are starting the core thing now\n"); fflush(stdout);
  int retVal = UIApplicationMain(NULL, NULL, nil, nil);
};
pthread_t appthread;
*/

UIWindow *window;
EAGLView *glView;


void Os::Init() {
  // hahahaha eeeeevil
  pool = [[NSAutoreleasePool alloc] init];  // this will never be released ever
  
  int rv = setjmp(sig);
  if(!rv) {
    char ax;
    char *tt = (char*)alloca(1024*768);
    printf("%d\n", tt - &ax);
    assert(&ax - tt > 1024 * 768);
    UIApplicationMain(NULL, NULL, NULL, @"GlopAppDelegate");
  }
  
  assert(rv == SIGNAL_LAUNCHED);
  
  [UIApplication sharedApplication].statusBarHidden = YES;
 
 	CGRect					rect = [[UIScreen mainScreen] applicationFrame];
	CGFloat					components[3];

	//Create a full-screen window
	window = [[EaterOfTouches alloc] initWithFrame:[UIScreen mainScreen].bounds];
	[window setBackgroundColor:[UIColor blueColor]];
  
  [window setMultipleTouchEnabled:YES];
  [window setExclusiveTouch:YES];
  
  glView = [[EAGLView alloc] initWithFrame:window.bounds];
	[window addSubview:glView];
  
  [window makeKeyAndVisible];
  //[glView makeKeyAndVisible];
  
  [NSTimer scheduledTimerWithTimeInterval:(NSTimeInterval)(1.0 / 1000) target:gad selector:@selector(timerYay) userInfo:nil repeats:TRUE];

  printf("ENTERING LOOP OF SATAN\n"); fflush(stdout);
  Think();
};
void Os::ShutDown() { };

void Os::Think() {
  while(true) {
    int rv = setjmp(sig);
    if(!rv)
      longjmp(rt, 1);
    
    if(rv != 5)
      printf("Signal %d\n", rv);
    
    if(rv == SIGNAL_TIMER)
      break;
  }
};
void Os::WindowThink(OsWindowData *window) { };

OsWindowData* Os::CreateWindow(const string &title, int x, int y, int width, int height,
                                    bool full_screen, short stencil_bits, const Image *icon,
                                    bool is_resizable) {
  return new OsWindowData();
}

void Os::DestroyWindow(OsWindowData *window) {
  delete window;
}

bool Os::IsWindowMinimized(const OsWindowData *window) {
  return false;
} // what

void Os::GetWindowFocusState(OsWindowData *window, bool *is_in_focus, bool *focus_changed) {
  *is_in_focus = true;
  *focus_changed = false;
}

void Os::GetWindowPosition(const OsWindowData *window, int *x, int *y) {
  *x = 0;
  *y = 0;
}
void Os::GetWindowSize(const OsWindowData *window, int *width, int *height) {
  *width = 320;
  *height = 480;
}
void Os::SetTitle(OsWindowData *window, const string &title) {
}
void Os::SetIcon(OsWindowData *window, const Image *icon) {
}

void Os::SetWindowSize(OsWindowData *window, int width, int height) {
}

vector<Os::KeyEvent> Os::GetInputEvents(OsWindowData *window) {
  vector<Os::KeyEvent> rv;
  rv.push_back(Os::KeyEvent(GetTime(), 0, 0, false, false));  // put the most recent cursor location in here at some point
  return rv;
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

vector<string> Os::ListFiles(const string &directory) {
  return vector<string>();
}
vector<string> Os::ListSubdirectories(const string &directory) {
  return vector<string>();
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




void Os::MessageBox(const string &title, const string &message) {
  printf("Message box: %s/%s\n", title.c_str(), message.c_str());
}
vector<pair<int,int> > Os::GetFullScreenModes() {
  vector<pair<int, int> > out;
  out.push_back(make_pair(320, 480));
  return out;
}

void Os::Sleep(int t) {
  usleep(t*1000);
}

int Os::GetTime() {
  return int(Os::GetTimeMicro() / 1000);
}

int64 Os::GetTimeMicro() {
  struct timeval tim;
  gettimeofday(&tim, NULL);
  return (int64)tim.tv_sec * 1000000 + tim.tv_usec;
}


int Os::GetRefreshRate() {
  return 60; // I don't even know
}

void Os::EnableVSync(bool is_enabled) {
  // is this possible?
}



void Os::SetCurrentContext(OsWindowData* window) {
}
void Os::SwapBuffers(OsWindowData *window) {
  [glView->context presentRenderbuffer:GL_RENDERBUFFER_OES];

  [glView rebind];
  
  glBindFramebufferOES(GL_FRAMEBUFFER_OES, glView->defaultFramebuffer);
  glViewport(0, 0, glView->backingWidth, glView->backingHeight);
  
  // This application only creates a single color renderbuffer which is already bound at this point.
  // This call is redundant, but needed if dealing with multiple renderbuffers.
  glBindRenderbufferOES(GL_RENDERBUFFER_OES, glView->colorRenderbuffer);
}

