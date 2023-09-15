#pragma once

#include <iostream>


struct double2 {
	double x;
	double y;

	double2() : x(), y() {}
	double2(double x, double y) : x(x), y(y) {}

	bool positive() const {return x > 0 && y > 0;}
};

double2 operator +(double2 a, double2 b) {
	return {a.x + b.x, a.y + b.y};
}

double2 operator *(double2 a, double b) {
	return {a.x * b, a.y * b};
}

std::ostream &operator <<(std::ostream &s, double2 value) {
	s << value.x << ' ' << value.y;
	return s;
}
