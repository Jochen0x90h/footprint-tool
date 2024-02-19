#pragma once

#include "double2.hpp"


struct double3 {
	double x;
	double y;
	double z;

	double3() : x(), y(), z() {}
	double3(double x, double y, double z) : x(x), y(y), z(z) {}
	double2 xy() const {return {x, y};}
};

inline double3 operator -(double3 a) {
	return {-a.x, -a.y, -a.z};
}

inline double3 operator +(double3 a, double3 b) {
	return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline double3 operator -(double3 a, double3 b) {
	return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline double3 operator *(double3 a, double b) {
	return {a.x * b, a.y * b, a.z * b};
}

inline double3 operator *(double3 a, double3 b) {
	return {a.x * b.x, a.y * b.y, a.z * b.z};
}

inline double3 operator /(double3 a, double b) {
	return {a.x / b, a.y / b, a.z / b};
}

inline std::ostream &operator <<(std::ostream &s, double3 value) {
	s << value.x << ' ' << value.y << ' ' << value.z;
	return s;
}
