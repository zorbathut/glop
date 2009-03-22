// Includes
#include "Base.h"
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdarg.h>

// Zero-dependency logging formatter and display message handler. These are used before
// System::Init is called.
static string ZeroDependencyLogFormatter(const char *filename, int line, const string &message) {
  return Format("[%s:%d] %s\n", filename, line, message.c_str());
}
static void ZeroDependencyFatalErrorHandler(const string &message) {
  printf("%s\n", message.c_str());
}

// Globals
static string (*gLogFormatter)(const char *filename, int line, const string &message) =
  ZeroDependencyLogFormatter;
static bool gLoggingStarted = false;
static bool gLogToStdErr = true;
static time_t gLogStartTime = time(0);
static string gLogFilename;
static void (*gLogFunction)(const string &) = NULL;
static FILE *gLogFile = 0;
static void (*gFatalErrorHandler)(const string &message) = ZeroDependencyFatalErrorHandler;

// Returns an STL string using printf style formatting.
string Format(const char *text, ...) {
  va_list arglist;
  va_start(arglist, text);
  string result = Format(text, arglist);
  va_end(arglist);
  return result;
}

string Format(const char *text, va_list arglist) {
  // We do not know how much space is required, so first try with an estimated amount of space.
  char buffer[1024];
#ifdef MSVC  // Thank you Visual C++. It is GOOD that you changed the name of this function.
  int length = _vscprintf(text, arglist);
  if (length < sizeof(buffer))
    vsprintf(buffer, text, arglist);
#else
  int length = vsnprintf(buffer, sizeof(buffer), text, arglist);
#endif
  if (length >= 0 && length < sizeof(buffer))
    return string(buffer);

  // Otherwise, allocate the necessary space and then do it
  char *var_buffer = new char[length + 1];
  vsprintf(var_buffer, text, arglist);
  string result = string(var_buffer);
  delete[] var_buffer;
  return result;
}

// Logging utilities
// =================

void SetLogFormatter(string (*formatter)(const char *filename, int line, const string &message)) {
  gLogFormatter = formatter;
}

void LogToFile(const string &filename, bool also_log_to_std_err) {
  ASSERT(gLoggingStarted == false);
  gLogFilename = filename;
  gLogToStdErr = also_log_to_std_err;
}

void LogToFunction(void (*func)(const string &), bool also_log_to_std_err) {
  ASSERT(gLoggingStarted == false);
  gLogFunction = func;
  gLogToStdErr = also_log_to_std_err;
}

void __Log(const char *filename, int line, const string &message) {
  // Open the log file if this is our first call
  if (!gLoggingStarted && gLogFilename != "") {
    gLogFile = fopen(gLogFilename.c_str(), "wt");
    ASSERT(gLogFile != 0);
  }
  if (!gLoggingStarted) {
    string clock_time = ctime(&gLogStartTime);
    string temp = Format("Program started at: %s", clock_time.c_str());
    if (gLogFile != 0)
      fputs(temp.c_str(), gLogFile);
    if (gLogToStdErr)
      fputs(temp.c_str(), stderr);
  }
  gLoggingStarted = true;

  // Prune off the directory of the filename
  int index = (int)strlen(filename);
  while (index > 0 && filename[index-1] != '/' && filename[index-1] != '\\')
    index--;
  const char *pruned_filename = filename + index;

  // Output the log message
  string formatted_message = gLogFormatter(pruned_filename, line, message);
  if (gLogFile != 0)
    fputs(formatted_message.c_str(), gLogFile);
  if (gLogToStdErr)
    fputs(formatted_message.c_str(), stderr);
  if (gLogFunction)
    (*gLogFunction)(formatted_message); // We are binary-safe.
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

void SetFatalErrorHandler(void (*handler)(const string &message)) {
  gFatalErrorHandler = handler;
}

void FatalError(const string &error) {
  gFatalErrorHandler(error);
  exit(-1);
}

void FatalErrorf(const char *error, ...) {
  va_list arglist;
  va_start(arglist, error);
  FatalError(Format(error, arglist));
  va_end(arglist);
}

// Handles a failed assertion. It returns an integer so that the ASSERT macro can be done with a ?:
// operator and thus require a semi-colon.
int __AssertionFailure(const char *filename, int line, const char *expression) {
  FatalError(Format("Assertion failed on line #%d of file %s:\n\n%s.",
                    line, filename, expression));
  return 1;
}
