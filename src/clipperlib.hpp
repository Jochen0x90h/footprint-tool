#pragma once

#include "double2.hpp"
#include <clipper2/clipper.h>


// Clipper2 Source: https://github.com/AngusJohnson/Clipper2
// Demo: https://jsclipper.sourceforge.net/6.2.1.0/main_demo.html


namespace clipperlib = Clipper2Lib;

inline clipperlib::Point<int64_t> toClipperPoint(double2 p) {
	return {int64_t(std::round(p.x * 1000.0)), int64_t(std::round(p.y * 1000))};
}

inline double2 toPoint(clipperlib::Point<int64_t> p) {
	return {double(p.x) * 0.001, double(p.y) * 0.001};
}
