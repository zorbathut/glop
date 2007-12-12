// A simple RGBA color stored as an array of 4 floats. We use this rather than unsigned characters
// since several OpenGl commands require float arrays as colors (eg glFog).
// Note that arithmetic operations ignore alpha values. This is so both kWhite / 2 and
// kWhite/2 + kRed/2 work and have alpha = 1.

#ifndef GLOP_COLOR_H__
#define GLOP_COLOR_H__

// Includes
#include "Base.h"

// Color class definition
class Color {
 public:
  // Constructors
  Color() {}
  Color(float red, float green, float blue, float alpha = 1) {
    data_[0] = red;
    data_[1] = green;
    data_[2] = blue;
    data_[3] = alpha;
  }
  Color(const Color &rhs) {operator=(rhs);}
  void operator=(const Color &rhs) {
    data_[0] = rhs.data_[0];
    data_[1] = rhs.data_[1];
    data_[2] = rhs.data_[2];
    data_[3] = rhs.data_[3];
  }
 
  // Accessors
  float operator[](int index) const {return data_[index];}
  float &operator[](int index) {return data_[index];}
  const float *GetData() const {return data_;}
  float *GetData() {return data_;}

  // Pointwise addition - does not alter alpha value
  const Color &operator+=(const Color &rhs) {
    data_[0] += rhs.data_[0];
    data_[1] += rhs.data_[1]; 
    data_[2] += rhs.data_[2];
    return *this;
  }
  const Color &operator-=(const Color &rhs) {
    data_[0] -= rhs.data_[0];
    data_[1] -= rhs.data_[1]; 
    data_[2] -= rhs.data_[2];
    return *this;
  }
  Color operator+(const Color &rhs) const {
    Color temp(*this);
    return (temp += rhs);
  }
  Color operator-(const Color &rhs) const {
    Color temp(*this);
    return (temp -= rhs);
  }
  Color operator-() const {return Color(-data_[0], -data_[1], -data_[2], -data_[3]);}
  
  // Scalar multiplication - does not scale alpha value
  const Color &operator*=(float scale) {
    data_[0] *= scale;
    data_[1] *= scale; 
    data_[2] *= scale;
    return *this;
  }
  const Color &operator/=(float scale) {
    data_[0] /= scale;
    data_[1] /= scale; 
    data_[2] /= scale;
    return *this;
  }
  Color operator*(float scale) const {
    Color temp(*this);
    return (temp *= scale);
  }
  Color operator/(float scale) const {
    Color temp(*this);
    return (temp /= scale);
  }
  
  // Comparators
  bool operator<(const Color &rhs) const {
    if (!IsEqual(data_[0], rhs.data_[0])) return data_[0] < rhs.data_[0];
    if (!IsEqual(data_[1], rhs.data_[1])) return data_[1] < rhs.data_[1];
    if (!IsEqual(data_[2], rhs.data_[2])) return data_[2] < rhs.data_[2];
    return IsLess(data_[3], rhs.data_[3]);
  }
  bool operator==(const Color &rhs) const {
    return (IsEqual(data_[0], rhs.data_[0]) &&
            IsEqual(data_[1], rhs.data_[1]) &&
            IsEqual(data_[2], rhs.data_[2]) &&
            IsEqual(data_[3], rhs.data_[3]));
  }
  bool operator!=(const Color &rhs) const {return !operator==(rhs);}
  
 private:
  float data_[4];
};

// Global functions
inline Color operator*(float scale, const Color &rhs) {return rhs * scale;}

// Constants
const Color kWhite(1.0f, 1.0f, 1.0f);
const Color kBlack(0.0f, 0.0f, 0.0f);
const Color kRed(1.0f, 0.0f, 0.0f);
const Color kGreen(0.0f, 1.0f, 0.0f);
const Color kBlue(0.0f, 0.0f, 1.0f);
const Color kYellow(1.0f, 1.0f, 0.0f);
const Color kPurple(1.0f, 0.0f, 1.0f);
const Color kCyan(0.0f, 1.0f, 1.0f);

#endif
