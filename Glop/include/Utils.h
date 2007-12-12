// Random Glop functionality that does not fit clearly into any other location. This is
// distinguished from Base.h in that these utilities are useful but not absolutely essential.

#ifndef GLOP_UTILS_H__
#define GLOP_UTILS_H__

// Includes
#include "Base.h"
#include <stdarg.h>
#include <vector>

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

// Binary search utilities. Assumes that a list is ordered from least to greatest. BSFindLowerBound
// finds the largest index i such that v[i] <= target, or -1 if there is no such index. BSFindMatch
// finds the largest index i such that v[i] == target, or -1 if there is no such index.
template<class T> int BSFindMatch(const vector<T> &v, const T &target) {
  int i = BSFindLowerBound(v, target);
  if (i == -1 || v[i] != target)
    return -1;
  else
    return i;
}
template<class T> int BSFindLowerBound(const vector<T> &v, const T &target) {
  int lb = 0, ub = (int)v.size() - 1;
  if (v[lb] > target)
    return -1;
  while (lb < ub) {
    int i = (lb + ub + 1) / 2;
    if (v[i] > target)
      ub = i - 1;
    else
      lb = i;
  }
  return lb;
}

#endif // GLOP_UTILS_H__
