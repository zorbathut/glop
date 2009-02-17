// Basic Glop utilities to be included everywhere (or almost everywhere).
//
// This file (and the corresponding cpp file) intentionally depend on nothing else within Glop.
// However, a couple functions - namely FatalError and LOGF - are naturally improved by using other
// things within Glop. To deal with this, they both delegate to client-specified callbacks.
// Initially these are simple, but once System::Init has been called, they are automatically
// replaced with the full versions.

#ifndef GLOP_BASE_H__
#define GLOP_BASE_H__

// Includes
#include <stdarg.h>
#include <string>
using namespace std;

// Type definitions
#ifdef MSVC
typedef __int64 int64;
typedef unsigned __int64 uint64;
#else
typedef long long int64;
typedef unsigned long long uint64;
#endif

// Constants
const float kPi = 3.1415926535897932385f;
const float kJustifyLeft = 0.0f;
const float kJustifyCenter = 0.5f;
const float kJustifyRight = 1.0f;
const float kJustifyTop = 0.0f;
const float kJustifyBottom = 1.0f;

// Float utilities. Compare two floating point values while ignoring potential rounding errors.
inline bool IsLess(float lhs, float rhs) {return lhs < rhs - 1e-6;}
inline bool IsEqual(float lhs, float rhs) {return !IsLess(lhs, rhs) && !IsLess(rhs, lhs);}
inline bool IsGreater(float lhs, float rhs) {return IsLess(rhs, lhs);}

// Format is similar to sprintf except it outputs into a string, and it does not require special
// memory allocation. The second version makes it easy to implement other functions like this.
// (See the implementation of FatalMessagef for example.)
string Format(const char *text, ...);
string Format(const char *text, va_list arglist);

// Logging magic. These functions output a message to the log with a specified filename and line
// number. The logging macros below should be used instead. The logf tool is more complicated
// since there is no standardized way of doing macros with a variable number of arguments.
void __Log(const char *filename, int line, const string &message);
struct __LogfObject {
  __LogfObject(const char *_filename, int _line): filename(_filename), line(_line) {}
  void __Logf(const char *message, ...);
  const char *filename;
  int line;
};

// Actual logging utilities. By default, logging messages go to the console. LogToFile can be used
// to log to a file also/instead. LogToFile must be called before any log messages are generated.
// LOG and LOGF output messages to the log.
//
// Examples: LOG("Test"); LOGF("x + y = %d", x+y);
//
// SetLogFormatter can be used to change the appearance of log messages. By default, they display
// the file, line, message, and frames/time elapsed since System::Init was called. If System::Init
// has not been called, the latter two are omitted.
void SetLogFormatter(string (*formatter)(const char *filename, int line, const string &message));
void LogToFile(const string &filename, bool also_log_to_std_err = false);
void LogToFunction(void (*func)(const string &), bool also_log_to_std_err = false);

#define LOG(message) __Log(__FILE__, __LINE__, message)
#define LOGF __LogfObject(__FILE__, __LINE__).__Logf

// Error-handling utilities. FatalError normally outputs a message and then quits regardless.
// ASSERT generates a fatal error unless some expression evaluates to true.
// __AssertionFailure is just more macro magic, and should not be used directly.
//
// SetFatalErrorHandler can be used to change the way in which fatal error messages are output. By
// default, it uses System::DisplayMessage if System::Init has been called. Otherwise, it uses
// printf.
void SetFatalErrorHandler(void (*handler)(const string &message));
void FatalError(const string &error);
void FatalErrorf(const char *error, ...);
int __AssertionFailure(const char *filename, int line, const char *expression);
#define ASSERT(expression) \
  (void)((expression) != 0? 0 : __AssertionFailure(__FILE__, __LINE__, #expression))

// Disallow evil constructors (based on Google.com's basictypes.h)
// Place this in the private section of a class to declare a dummy copy constructor and a
// dummy assignement operation.
#define DISALLOW_EVIL_CONSTRUCTORS(TypeName) \
  TypeName(const TypeName&);                 \
  void operator=(const TypeName&)

#endif  // GLOP_BASE_H__
