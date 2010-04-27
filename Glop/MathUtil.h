#ifndef MATHUTIL_H__
#define MATHUTIL_H__

template <class T>
inline T Sqr(T val) {
	return val * val;
}

template <class T>
inline T DistSquared(T x1, T y1, T x2, T y2) {
	return Sqr(x2 - x1) + Sqr(y2 - y1);
}

#endif //MATHUTIL_H__
