#pragma once

#include <iostream>


struct double2 {
	double x;
	double y;

	double2() : x(), y() {}
	double2(double x, double y) : x(x), y(y) {}

	bool positive() const {return x > 0 && y > 0;}
	bool zero() const {return x == 0 && y == 0;}
};

inline double2 operator -(double2 a) {
	return {-a.x, -a.y};
}

inline double2 &operator +=(double2 &a, double2 b) {
	a.x += b.x;
	a.y += b.y;
	return a;
}

inline double2 operator +(double2 a, double2 b) {
	return {a.x + b.x, a.y + b.y};
}

inline double2 &operator -=(double2 &a, double2 b) {
	a.x -= b.x;
	a.y -= b.y;
	return a;
}

inline double2 operator -(double2 a, double2 b) {
	return {a.x - b.x, a.y - b.y};
}

inline double2 operator *(double2 a, double b) {
	return {a.x * b, a.y * b};
}

inline double2 operator *(double2 a, double2 b) {
	return {a.x * b.x, a.y * b.y};
}

inline std::ostream &operator <<(std::ostream &s, double2 value) {
	s << value.x << ' ' << value.y;
	return s;
}

inline double2 min(double2 a, double2 b) {
	return {std::min(a.x, b.x), std::min(a.y, b.y)};
}

inline double2 max(double2 a, double2 b) {
	return {std::max(a.x, b.x), std::max(a.y, b.y)};
}

inline double length(double2 a) {
	return sqrt(a.x * a.x + a.y * a.y);
}

inline double2 normalize(double2 a) {
	double s = 1.0 / length(a);
	return {a.x * s, a.y * s};
}