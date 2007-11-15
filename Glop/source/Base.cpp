// Includes
#include "Base.h"
#include "Os.h"
#include "System.h"
#include "Utils.h"
#include <stdarg.h>

// Constants
const char *const kDefaultLogFilename = "log.txt";

// Globals
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

void LogToFile(const string &filename) {
  ASSERT(gLogFilename == "" && gLogFile == 0);
  gLogFilename = filename;
}

void LogToStdErr() {
  ASSERT(gLogFilename == "" && gLogFile == 0);
  gLogFile = stderr;
}

void __Log(const char *filename, int line, const string &message) {
  // Open the log file if this is our first call
  if (gLogFile == 0) {
    if (gLogFilename == "")
      gLogFilename = kDefaultLogFilename;
    gLogFile = fopen(gLogFilename.c_str(), "wt");
    ASSERT(gLogFile != 0);
  }

  // Output the log message
  int frame_count = gSystem->GetFrameCount();
  int ticks = gSystem->GetTime();
  string temp = Format("[%df %.3fs %s:%d] %s\n", frame_count, ticks / 1000.0f,
                       filename, line, message.c_str());
  fputs(temp.c_str(), gLogFile);
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
