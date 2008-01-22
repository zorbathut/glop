// Basic Glop utilities to be included everywhere (or almost everywhere).

#ifndef GLOP_BASE_H__
#define GLOP_BASE_H__

// Includes
#include <string>
using namespace std;

// Type definitions
typedef int LightSetId;
#ifdef WIN32
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
// memory allocation.
string Format(const char *text, ...);

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
// Examples: LOG("Test"); LOGF("x + y = %d", x+y);
void LogToFile(const string &filename, bool also_log_to_std_err);
#define LOG(message) __Log(__FILE__, __LINE__, message)
#define LOGF __LogfObject(__FILE__, __LINE__).__Logf

// Error-handling utilities. DisplayMessage outputs a highly visible message to the user - the
// exact form depends on the operating system. FatalError generates such a message and then quits.
// ASSERT generates a fatal error unless some expression evaluates to true.
// __AssertionFailure is just more macro magic, and should not be used directly.
void DisplayMessage(const string &title, const string &message);
void DisplayMessagef(const string &title, const char *message, ...);
void FatalError(const string &error);
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
