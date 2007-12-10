#ifndef MATH_H__
#define MATH_H__

template <class T>
__forceinline T Sqr(T val) {
	return val * val;
}

template <class T>
__forceinline T DistSquared(T x1, T y1, T x2, T y2) {
	return Sqr(x2 - x1) + Sqr(y2 - y1);
}

#endif //MATH_H__