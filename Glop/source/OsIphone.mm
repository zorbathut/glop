
#include "os.h"

#include <sys/time.h>

#import <UIKit/UIKit.h>
#import "OsIphone_EAGLView.h"

struct OsWindowData { };

static jmp_buf hatred; // THIS IS THE MOST ACCURATELY NAMED VARIABLE IN THIS ENTIRE CODEBASE

// *********************************************
//  *                     GlopAppDelegate
@interface GlopAppDelegate : NSObject <UIApplicationDelegate> {
}
@end

@implementation GlopAppDelegate
- (void) applicationDidFinishLaunching:(UIApplication *)application
{
  printf("did finish launching, creating evil\n"); fflush(stdout);
  longjmp(hatred, 1);
}

- (void) applicationWillResignActive:(UIApplication *)application
{
}

- (void) applicationDidBecomeActive:(UIApplication *)application
{
}

- (void)applicationWillTerminate:(UIApplication *)application
{
  printf("terminate hit\n"); fflush(stdout);
}
@end


GlopAppDelegate *glopdelegate;
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
  if(!setjmp(hatred)) {
    UIApplicationMain(NULL, NULL, NULL, @"GlopAppDelegate");
  }
  
  printf("we made it\n"); fflush(stdout);
  
  [UIApplication sharedApplication].statusBarHidden = YES;
 
 	CGRect					rect = [[UIScreen mainScreen] applicationFrame];
	CGFloat					components[3];

	//Create a full-screen window
	window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
	[window setBackgroundColor:[UIColor blueColor]];
  
  glView = [[EAGLView alloc] initWithFrame:rect];
	[window addSubview:glView];
};
void Os::ShutDown() { };

void Os::Think() {
  printf("thinking 1\n"); fflush(stdout);
  SInt32 rv;
  do {
    printf("thinking 2\n"); fflush(stdout);
    rv = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, TRUE);
    printf("thinking 3\n"); fflush(stdout);
  } while(rv == kCFRunLoopRunHandledSource);
  printf("thinking 4\n"); fflush(stdout);
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

void Os::SwapBuffers(OsWindowData *window) {
  // hmm
}

void Os::SetCurrentContext(OsWindowData* window) {
}
