#pragma once

#include "double2.hpp"
#include <clipper2/clipper.h>


// Clipper2 Source: https://github.com/AngusJohnson/Clipper2
// Demo: https://jsclipper.sourceforge.net/6.2.1.0/main_demo.html


namespace clipper2 = Clipper2Lib;

constexpr double clipperFactor = 1000.0;

inline int64_t toClipperValue(double v) {
	return int64_t(std::round(v * clipperFactor));
}

inline clipper2::Point64 toClipperPoint(const double2 &p) {
	return {toClipperValue(p.x), toClipperValue(p.y)};
}

inline double2 toPoint(const clipper2::Point64 &p) {
	return {double(p.x) / clipperFactor, double(p.y) / clipperFactor};
}
