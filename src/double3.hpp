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
