// Includes
#include "Utils.h"
#include "Base.h"
#include <stdio.h>
#include <limits.h>

// Internal function that does a string printf from a vararg.
string Format(const char *text, va_list arglist) {
  // We do not know how much space is required, so first try with an estimated amount of space.
  char buffer[1024];
#ifdef WIN32  // Thank you Visual C++. It is GOOD that you changed the name of this function.
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

// String parsing
// ==============

// Converts this string to a bool. Method #1 assumes valid formatting, method #2 returns
// whether the data is correctly formatted, and gives the value via *result.
// *BoolFormat* = true || false
bool ToBool(const string &s) {
  bool result;
  ASSERT(ToBool(s, &result));
  return result;
}
bool ToBool(const string &s, bool *result) {
  if (s == "true") {
    *result = true;
    return true;
  } else if (s == "false") {
    *result = false;
    return true;
  } else {
    return false;
  }
}

// Converts this string to a char. Method #1 assumes valid formatting, method #2 returns
// whether the data is correctly formatted, and gives the value via *result.
// *CharFormat* = '\\' || '\'' || '\n' || '\r' || '\t' || '?' (for ? = ASCII character) ||
//                *IntFormat* between -128 and 256.
// See ToInt.
char ToChar(const string &s) {
  char result;
  ASSERT(ToChar(s, &result));
  return result;
}
bool ToChar(const string &s, char *result) {
  // Handle characters surrounded by ' markers
  if (s == "'\\'") *result = '\\';
  else if (s == "'\''") *result = '\'';
  else if (s == "'\n'") *result = '\n';
  else if (s == "'\r'") *result = '\r';
  else if (s == "'\t'") *result = '\t';
  else if (s[0] == '\'' && s[2] == '\'' && s.size() == 3) *result = s[1];
  
  // Handle ASCII codes
  else {
    int temp;
    if (ToInt(s, &temp) && temp >= 0 && temp < 256) {
      *result = (char)temp;
      return true;
    }
    return false;
  }
  
  // Return success for recognized 'X' characters
  return true;
}

// Converts this string to a double. Method #1 assumes valid formatting, method #2 returns
// whether the data is correctly formatted, and gives the value via *result.
// *DoubleFormat* = 0 | (-)Digit*(.)Digit*
double ToDouble(const string &s) {
  return atof(s.c_str());
}
bool ToDouble(const string &s, double *result) {
  int start_pos = 0;
  
  // Account for negative signs and empty strings and leading zeroes
  if (s.size() == 0)
    return false;
  if (s[start_pos] == '-')
    start_pos++;
  if ((int)s.size() > start_pos+1 && s[start_pos] == '0' && s[start_pos+1] != '.')
    return false;
  
  // Scan each remaining digit to see if this is a valid float
  bool hit_decimal = false;
  for (int pos = start_pos; pos < (int)s.size(); pos++)
    if (s[pos] < '0' || s[pos] > '9') {
      if (s[pos] == '.' && !hit_decimal) 
        hit_decimal = true;
      else
        return false;
    }
  
  // Everything is fine (except possibly -0) - convert to a double and return
  *result = atof(s.c_str());
  if (*result == 0 && s[0] == '-')
    return false;
  return true;
}
bool ToFloat(const string &s, float *result) {
  double double_result;
  if (!ToDouble(s, &double_result))
    return false;
  *result = (float)double_result;
  return true;
}

// Converts this string to an int. Method #1 assumes valid formatting, method #2 returns
// whether the data is correctly formatted (and in range), and gives the value via *result.
//  *IntFormat* = Digit* | - NonZeroDigit Digit*
// Note this allows leading zeroes on positive numbers.
int ToInt(const string &s, int base) {
  int result;
  ASSERT(ToInt(s, &result, base));
  return result;
}
bool ToInt(const string &s, int *result, int base) {
  const int64 kMaxInt = 2147483647L, kMinInt = -2147483647L - 1;
    
  // Account for negative signs and empty strings and leading zeroes
  int pos = 0;
  bool is_negative = false;
  if (s[pos] == '-') {
    is_negative = true;
    pos++;
  }
  if (s[pos] == 0 || (is_negative && s[pos] == '0'))
    return false;

  // Get the value
  int64 value = 0;
  while (s[pos] != 0) {   
    int digit;
    if (s[pos] >= '0' && s[pos] <= '9' && s[pos] < '0'+base)
      digit = s[pos] - '0';
    else if (s[pos] >= 'A' && s[pos] < 'A'+base-10)
      digit = s[pos] + 10 - 'A';
    else if (s[pos] >= 'a' && s[pos] < 'a'+base-10)
      digit = s[pos] + 10 - 'a';
    else
      return false;
    pos++;
    value = base*value + digit;
    if ( (!is_negative && value > kMaxInt) || (is_negative && value > kMinInt) )
      return false;
  }
  if (is_negative)
    value = -value;
  *result = (int)value;
  return true;
}

// Converts this string to an int. Method #1 assumes valid formatting, method #2 returns
// whether the data is correctly formatted (and in range), and gives the value via *result.
// *ShortFormat* = *IntFormat*
bool ToShort(const string &s, short *result, int base) {
  const int kShortMin = -32768, kShortMax = 32767;
  int temp;
  if (!ToInt(s, &temp, base))
    return false;
  if (temp >= kShortMin && temp <= kShortMax) {
    *result = (short)temp;
    return true;
  } else
    return false;
}
