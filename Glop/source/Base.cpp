// Includes
#include "Base.h"
#include "System.h"
#include "Utils.h"
#include "Os.h"
#include <ctime>
#include <stdarg.h>

#include <stdio.h>

// Logging globals
bool gLoggingStarted = false;
bool gLogToStdErr = true;
time_t gLogStartTime = time(0);
string gLogFilename;
FILE *gLogFile = 0;

// Returns an STL string using printf style formatting.
string Format(const char *text, ...) {
  va_list arglist;
  va_start(arglist, text);
  string result = Format(text, arglist);
  va_end(arglist);
  return result;
}

// Logging utilities
// =================

void LogToFile(const string &filename, bool also_log_to_std_err) {
  ASSERT(gLoggingStarted == false);
  gLogFilename = filename;
  gLogToStdErr = also_log_to_std_err;
}

void __Log(const char *filename, int line, const string &message) {
  // Open the log file if this is our first call
  printf("here\n");
  if (!gLoggingStarted && gLogFilename != "") {
    gLogFile = fopen(gLogFilename.c_str(), "wt");
    ASSERT(gLogFile != 0);
  }
  printf("2\n");
  if (!gLoggingStarted) {
    string clock_time = ctime(&gLogStartTime);
    string temp = Format("Program started at: %s", clock_time.c_str());
    if (gLogFile != 0)
      fputs(temp.c_str(), gLogFile);
    if (gLogToStdErr)
      fputs(temp.c_str(), stderr);
  }
  printf("3\n");
  gLoggingStarted = true;

  // Output the log message
  int frame_count = 0;//system()->GetFrameCount();
  int ticks = system()->GetTime();
  int index = (int)strlen(filename);
  printf("4\n");
  const char *temp_filename = filename + strlen(filename) - 1;
  while (index > 0 && filename[index-1] != '/' && filename[index-1] != '\\')
    index--;
  string temp = Format("[f%d %.3fs %s:%d] %s\n", frame_count, ticks / 1000.0f,
                       filename+index, line, message.c_str());
  printf("5\n");
  if (gLogFile != 0)
    fputs(temp.c_str(), gLogFile);
  if (gLogToStdErr)
    fputs(temp.c_str(), stderr);
  printf("6\n");
  fflush(gLogFile);
}

void __LogfObject::__Logf(const char *message, ...) {
  va_list arglist;
  va_start(arglist, message);
  __Log(filename, line, Format(message, arglist));
  va_end(arglist);
}

// Error-handling utilities
// ========================

void DisplayMessage(const string &title, const string &message) {
  Os::DisplayMessage(title, message);
}

void DisplayMessagef(const string &title, const char *message, ...) {
  va_list arglist;
  va_start(arglist, message);
  DisplayMessage(title, Format(message, arglist));
  va_end(arglist);
}

void FatalError(const string &error) {
  Os::DisplayMessage("Fatal Error", error);
  exit(-1);
}

// Handles a failed assertion. It returns an integer so that the ASSERT macro can be done with a ?:
// operator and thus require a semi-colon.
int __AssertionFailure(const char *filename, int line, const char *expression) {
  FatalError(Format("Assertion failed on line #%d of file %s:\n\n%s.",
                    line, filename, expression));
  return 1;
}
