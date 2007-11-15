// Random Glop functionality that does not fit clearly into any other location. This is
// distinguished from Base.h in that these utilities are useful but not absolutely essential.

#ifndef GLOP_UTILS_H__
#define GLOP_UTILS_H__

// Includes
#include "Base.h"
#include <stdarg.h>

// Format function that takes a variable argument parameter. Functions that wish to use an
// interface similar to printf should use this.
string Format(const char *text, va_list arglist);

// String parsing utilities. The ToX functions return whether the string is validly formatted
// in the given form if the first option is used. If the second option is used, the string is
// assumed to be validly formatted. See Utils.cpp for the exact format used.
bool ToBool(const string &s);
bool ToBool(const string &s, bool *result);
char ToChar(const string &s);
bool ToChar(const string &s, char *result);
double ToDouble(const string &s);
bool ToDouble(const string &s, double *result);
inline float ToFloat(const string &s) {return float(ToDouble(s));}
bool ToFloat(const string &s, float *result);
int ToInt(const string &s, int base = 10);
bool ToInt(const string &s, int *result, int base = 10);
inline short ToShort(const string &s, int base = 10) {return short(ToInt(s, base));}
bool ToShort(const string &s, short *result, int base = 10);

#endif // GLOP_UTILS_H__
