#include "clipper2.hpp"
#include "double3.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <set>


using json = nlohmann::json;
namespace fs = std::filesystem;



// shapes
constexpr double CIRCLE = 0.5; // oval if width not equal to height
constexpr double ROUNDRECT = 0.25; // 25% (KiCad default)
constexpr double ROUNDRECT10 = 0.1; // 10%
constexpr double ROUNDRECT5 = 0.05; // 5%
constexpr double RECT = 0;


struct Footprint {
	enum class Type {
		DETECT,
		THROUGH_HOLE,
		SMD,
	};

	enum class Numbering {
		CIRCULAR,
		ZIGZAG,
		DOUBLE
	};

	// pad or pad array
	struct Pad {
		enum class Type {
			// single line of pads
			SINGLE,

			// dual pad lines
			DUAL,

			// quad pad lines (square or rectangular)
			QUAD,

			// matrix of pads
			MATRIX
		};

		// global position of pad or center of multiple pads
		double2 position;

		// size of pad
		double2 size;

		// offset of pad relative to position
		double2 offset;

		// shape of pad
		double shape = ROUNDRECT;

		// size of drill
		double2 drillSize;

		// offset of drill relative to position
		double2 drillOffset;

		// clearance
		double clearance = 0;

		// solder mask margin
		double maskMargin = 0;

		// layer
		bool back = false;


		// package type for generating multiple pads
		Type type = Type::SINGLE;

		// pitch between pads
		double pitch = 0;

		// distance between pad rows
		double2 distance;

		// number of pads
		int count = 1;

		// mirror pads (pin 1 right instead of left)
		bool mirror = false;

		// numbering scheme
		Numbering numbering = Numbering::CIRCULAR;

		// number of first pad
		int number = 1;

		// pad number increment
		int increment = 1;

		// pad names (override numbers)
		std::vector<std::string> names;


		// pad numbers to exclude
		//std::set<int> exclude;

		//bool hasNumber(int number) const {
	//		return !this->exclude.contains(number);
		//}

		bool exists(int index) const {
			return index >= this->names.size() || !this->names[index].empty();
		}

		std::string getName(int index) const {
			if (index >= this->names.size()) {
				return std::to_string(this->number + index * this->increment);
			} else {
				return this->names[index];
			}
		}
	};

	bool template_ = false;
	std::string name;
	std::string description;

	// through-hole or smd
	Type type = Type::DETECT;

	// body size, used for silkscreen, courtyard and 3D model
	double3 body;

	// additional courtyard margin
	double2 margin;

	// generate silkscreen
	bool silkscreen = true;

	// global position
	double2 position;

	// list of pads (pad arrays)
	std::vector<Pad> pads;

	Type getType() const {
		if (this->type == Footprint::Type::DETECT) {
			// detect footprint type
			for (auto &pad : this->pads) {
				if (pad.size.positive() && pad.drillSize.positive())
					return Type::THROUGH_HOLE;
			}
			return Type::SMD;
		}
		return this->type;
	}
};

void read(json &j, const std::string &key, std::string &value) {
	value = j.value(key, value);
}

void read(json &j, const std::string &key, bool &value) {
	value = j.value(key, value);
}

void read(json &j, const std::string &key, int &value) {
	value = j.value(key, value);
}

void read(json &j, const std::string &key, double &value) {
	value = j.value(key, value);
}

void readRelaxed(json &j, const std::string &key, double2 &value) {
	if (j.contains(key)) {
		json jv = j.at(key);
		if (jv.is_number()) {
			value.x = jv.get<double>();
			value.y = value.x;
		} else if (jv.is_array()) {
			value.x = jv.at(0).get<double>();
			if (jv.size() >= 2)
				value.y = jv.at(1).get<double>();
			else
				value.y = value.x;
		}
	}
}

void read(json &j, const std::string &key, double2 &value) {
	if (j.contains(key)) {
		json jv = j.at(key);
		value.x = jv.at(0).get<double>();
		value.y = jv.at(1).get<double>();
	}
}

void read(json &j, const std::string &key, double3 &value) {
	if (j.contains(key)) {
		json jv = j.at(key);
		value.x = jv.at(0).get<double>();
		value.y = jv.at(1).get<double>();
		value.z = jv.at(2).get<double>();
	}
}


void readPad(json &j, Footprint::Pad &pad) {
	// position
	read(j, "position", pad.position);

	// size
	readRelaxed(j, "size", pad.size);

	// offset
	readRelaxed(j, "offset", pad.offset);

	// shape
	read(j, "shape", pad.shape);

	// drill size
	readRelaxed(j, "drillSize", pad.drillSize);

	// drill offset
	readRelaxed(j, "drillOffset", pad.drillOffset);

	// clearance
	read(j, "clearance", pad.clearance);

	// solder mask margin
	read(j, "maskMargin", pad.maskMargin);

	// back side
	read(j, "back", pad.back);


	// type
	std::string type = j.value("type", std::string());
	if (type == "dual")
		pad.type = Footprint::Pad::Type::DUAL;
	else if (type == "quad")
		pad.type = Footprint::Pad::Type::QUAD;
	else if (type == "matrix")
		pad.type = Footprint::Pad::Type::MATRIX;
	else
		pad.type = Footprint::Pad::Type::SINGLE;

	// pitch
	read(j, "pitch", pad.pitch);

	// distance
	readRelaxed(j, "distance", pad.distance);

	// pad count
	read(j, "count", pad.count);

	// mirror
	read(j, "mirror", pad.mirror);

	// numbering
	std::string numbering = j.value("numbering", std::string());
	if (numbering == "circular")
		pad.numbering = Footprint::Numbering::CIRCULAR;
	else if (numbering == "zigzag")
		pad.numbering = Footprint::Numbering::ZIGZAG;
	else if (numbering == "double")
		pad.numbering = Footprint::Numbering::DOUBLE;

	// first pad number
	read(j, "number", pad.number);

	// pad number increment
	read(j, "increment", pad.increment);

	// pad names
	if (j.contains("names")) {
		for (auto name : j.at("names")) {
			pad.names.push_back(name.get<std::string>());
		}
	}

	/*
	// exclude numbers
	if (j.contains("exclude")) {
		for (auto number : j.at("exclude")) {
			pad.exclude.insert(number.get<int>());
		}
	}*/

}

void readFootprint(json &j, std::map<std::string, Footprint> &footprints, Footprint &footprint) {
	// inherit existing footprint
	std::string inherit = j.value("inherit", std::string());
	if (footprints.contains(inherit)) {
		footprint = footprints[inherit];
		footprint.template_ = false;
	}

	// template
	read(j, "template", footprint.template_);

	// description
	read(j, "description", footprint.description);

	// body
	read(j, "body", footprint.body);

	// margin (enlarges silkscreen around body)
	readRelaxed(j, "margin", footprint.margin);

	read(j, "silkscreen", footprint.silkscreen);

	// global position
	read(j, "position", footprint.position);

	// pads/pad arrays
	if (j.contains("pads")) {
		auto jp = j.at("pads");

		int size = jp.size();
		footprint.pads.resize(size);
		for (int i = 0; i < size; ++i) {
			// read per-component values
			readPad(jp.at(i), footprint.pads[i]);
		}
	} else {
		// no pads
		footprint.pads.clear();
	}

	// type
	std::string type = j.value("type", std::string());
	if (type == "through hole")
		footprint.type = Footprint::Type::THROUGH_HOLE;
	else if (type == "smd")
		footprint.type = Footprint::Type::SMD;
}

void readJson(const fs::path &path, std::map<std::string, Footprint> &footprints) {
	// read config
	std::ifstream is(path.string());
	if (is.is_open()) {
		try {
			json j = json::parse(is);

			for (auto& [name, value] : j.items()) {
				Footprint footprint;

				try {
					readFootprint(value, footprints, footprint);
					footprints[name] = footprint;
				} catch (std::exception &e) {
					// parsing the json file failed
					std::cerr << name << ": " << e.what() << std::endl;
				}
			}
		} catch (std::exception &e) {
			// parsing the json file failed
			std::cerr << "json: " << e.what() << std::endl;
		}
	} else {
		std::cerr << "error: could not open file " << path.string() << std::endl;
	}
}

// define a pad
void generatePad(std::ofstream &s, std::string_view name, double2 position, double2 size, double shape, double2 drillSize, double2 drillOffset, double clearance, double maskMargin, bool back) {
	bool hasPad = size.positive();
	bool hasDrill = drillSize.positive();

	// pad
	if (hasPad) {
		s << "  (pad \"" << name << "\" ";
		s << (hasDrill ? "thru_hole" : "smd");
	} else {
		s << "  (pad \"\" np_thru_hole";
		shape = CIRCLE;
		size = drillSize;
	}

	// shape
	if (shape <= RECT)
		s << " rect";
	else if (shape >= CIRCLE)
		if (size.x == size.y)
			s << " circle";
		else
			s << " oval";
	else if (shape == ROUNDRECT)
		s << " roundrect";
	else
		s << " roundrect (roundrect_rratio " << shape << ")";

	// position/size
	s << " (at " << position << ") (size " << size << ")";

	// drill
	if (hasDrill) {
		s << " (drill ";
		if (drillSize.x == drillSize.y)
			s << drillSize.x;
		else
			s << "oval " << drillSize;
		if (!drillOffset.zero())
			s << " (offset " << drillOffset << ")";
		s << ")";
	}

	// margins
	if (clearance > 0)
		s << " (clearance " << clearance << ")";
	if (maskMargin != 0)
		s << " (solder_mask_margin " << maskMargin << ")";

	// layers
	const char *layers = hasDrill ? "*.Cu *.Mask" : (back ? "B.Cu B.Mask B.Paste" : "F.Cu F.Mask F.Paste");
	s << " (layers " << layers << "))" << std::endl;
}

void line(std::ofstream &s, double2 p1, double2 p2, double width, const char *layer) {
	s << "  (fp_line (start " << p1 << ") (end " << p2 << ") (width " << width << ") (layer " << layer << "))" << std::endl;
}


// draw a rectangle to courtyard layer
void rectangle(std::ofstream &s, double2 center, double2 size, double width, const char *layer) {
	double x1 = center.x - size.x * 0.5;
	double y1 = center.y - size.y * 0.5;
	double x2 = center.x + size.x * 0.5;
	double y2 = center.y + size.y * 0.5;
	line(s, {x1, y1}, {x2, y1}, width, layer);
	line(s, {x2, y1}, {x2, y2}, width, layer);
	line(s, {x2, y2}, {x1, y2}, width, layer);
	line(s, {x1, y2}, {x1, y1}, width, layer);
}

constexpr double silkscreenWidth = 0.15;
constexpr double silkscreenDistance = 0.1;
constexpr double padClearance = 0.1;

void addSilkscreenRectangle(clipper2::Clipper64 &clipper, double2 center, double2 size) {
	//size.x += silkscreenWidth + silkscreenDistance * 2;
	//size.y += silkscreenWidth + silkscreenDistance * 2;
	double x1 = center.x - size.x * 0.5;
	double y1 = center.y + size.y * 0.5;
	double x2 = center.x + size.x * 0.5;
	double y2 = center.y - size.y * 0.5;

	double d = 4 * silkscreenWidth;
	double x = x1 + (x2 > x1 ? d : -d);
	double y = y1 + (y2 > y1 ? d : -d);

	clipper2::Paths64 paths;
	{
		clipper2::Path64 &path = paths.emplace_back();
		path.push_back(toClipperPoint({x, y1}));
		path.push_back(toClipperPoint({x2, y1}));
		path.push_back(toClipperPoint({x2, y2}));
		path.push_back(toClipperPoint({x1, y2}));
		path.push_back(toClipperPoint({x1, y}));
	}

	// add pin1 indicator
	{
		clipper2::Path64 &path = paths.emplace_back();
		double w = silkscreenWidth * 0.5;
		path.push_back(toClipperPoint({x1 - w, y1 - w}));
		path.push_back(toClipperPoint({x1 + w, y1 - w}));
		path.push_back(toClipperPoint({x1 + w, y1 + w}));
		path.push_back(toClipperPoint({x1 - w, y1 + w}));
		path.push_back(path.front()); // close square becaue it is treated as open path
	}

	clipper.AddOpenSubject(paths);
}

inline void addSilkscreenPad(clipper2::Paths64 &paths, double2 center, double2 size, double2 drill) {
	size.x = std::max(size.x, drill.x);
	size.y = std::max(size.y, drill.y);
	size.x += silkscreenWidth + padClearance * 2;
	size.y += silkscreenWidth + padClearance * 2;
	double x1 = center.x - size.x * 0.5;
	double y1 = center.y + size.y * 0.5;
	double x2 = center.x + size.x * 0.5;
	double y2 = center.y - size.y * 0.5;

	clipper2::Path64 path;
	path.push_back(toClipperPoint({x1, y1}));
	path.push_back(toClipperPoint({x2, y1}));
	path.push_back(toClipperPoint({x2, y2}));
	path.push_back(toClipperPoint({x1, y2}));
	paths.push_back(path);
}


/*
void silkscreenRectangle(std::ofstream &s, double2 center, double2 size) {
	double x1 = center.x - size.x * 0.5;
	double y1 = center.y + size.y * 0.5;
	double x2 = center.x + size.x * 0.5;
	double y2 = center.y - size.y * 0.5;

	double d = 4 * silkscreenWidth;
	double x = x1 + (x2 > x1 ? d : -d);
	double y = y1 + (y2 > y1 ? d : -d);

	// pin 1 marking
	line(s, {x1, y1}, {x1, y1}, silkscreenWidth * 2, "F.SilkS");

	// remaining rectangle
	line(s, {x, y1}, {x2, y1}, silkscreenWidth, "F.SilkS");
	line(s, {x2, y1}, {x2, y2}, silkscreenWidth, "F.SilkS");
	line(s, {x2, y2}, {x1, y2}, silkscreenWidth, "F.SilkS");
	line(s, {x1, y2}, {x1, y}, silkscreenWidth, "F.SilkS");
}*/

void silkscreenPaths(std::ofstream &s, const clipper2::Paths64 &paths) {
	for (auto &path : paths) {
		int count = path.size();
		for (int i = 0; i < count - 1; ++i) {
			auto p1 = toPoint(path[i]);
			auto p2 = toPoint(path[(i + 1) % count]);
			line(s, p1, p2, silkscreenWidth, "F.SilkS");
		}
	}
}

constexpr double fabWidth = 0.15;
constexpr double fabDistance = 0.2;

void fabRectangle(std::ofstream &s, double2 center, double2 size) {
	double x1 = center.x - size.x * 0.5;
	double y1 = center.y + size.y * 0.5;
	double x2 = center.x + size.x * 0.5;
	double y2 = center.y - size.y * 0.5;

	double d = std::min(std::abs(size.x), std::abs(size.y)) * 0.25;
	double x = x1 + (x2 > x1 ? d : -d);
	double y = y1 + (y2 > y1 ? d : -d);

	line(s, {x, y1}, {x1, y}, silkscreenWidth, "F.Fab");
	line(s, {x, y1}, {x2, y1}, silkscreenWidth, "F.Fab");
	line(s, {x2, y1}, {x2, y2}, silkscreenWidth, "F.Fab");
	line(s, {x2, y2}, {x1, y2}, silkscreenWidth, "F.Fab");
	line(s, {x1, y2}, {x1, y}, silkscreenWidth, "F.Fab");
}

void generateSingle(std::ofstream &s, double2 globalPosition, const Footprint::Pad &pad, clipper2::Paths64 &clips) {
	int count = pad.count;

	// position of first pin
	double2 position = globalPosition + pad.position + double2((pad.pitch * (count - 1)) * -0.5, 0) + pad.offset;

	// drill offset
	double2 drillOffset = pad.drillOffset;

	// generate pins
	for (int i = 0; i < count; ++i) {
		//int number = pad.number + (pad.mirror ? count - 1 - i : i);
		int index = pad.mirror ? count - 1 - i : i;

		int n;
		switch (pad.numbering) {
		case Footprint::Numbering::CIRCULAR:
		case Footprint::Numbering::ZIGZAG:
			n = index;//pad.number + index * pad.increment;
			break;
		case Footprint::Numbering::DOUBLE:
			n = index / 2;//pad.number + (index / 2) * pad.increment;
			break;
		}


		//if (pad.hasNumber(n)) {
		if (pad.exists(n)) {
			generatePad(s, pad.getName(n), position, pad.size, pad.shape, pad.drillSize, drillOffset, pad.clearance, pad.maskMargin, pad.back);
			addSilkscreenPad(clips, position, pad.size, pad.drillSize);
		}
		position.x += pad.pitch;
	}
}

void generateDual(std::ofstream &s, double2 globalPosition, const Footprint::Pad &pad, clipper2::Paths64 &clips) {
	int count = pad.count / 2;

	double padDistance = pad.distance.x;

	// position of first pin
	double2 position1 = globalPosition + pad.position + double2((pad.pitch * (count - 1)) * -0.5, padDistance * 0.5) + pad.offset;
	double2 position2 = globalPosition + pad.position + double2((pad.pitch * (count - 1)) * -0.5, padDistance * -0.5) - pad.offset;

	// drill offset
	double2 drillOffset1 = pad.drillOffset;
	double2 drillOffset2 = -drillOffset1;

	// generate pins
	for (int i = 0; i < count; ++i) {
		int index = pad.mirror ? count - 1 - i : i;

		int n1, n2;
		switch (pad.numbering) {
		case Footprint::Numbering::CIRCULAR:
			//n1 = pad.number + index * pad.increment;
			//n2 = pad.number + (pad.count - 1 - index) * pad.increment;
			n1 = index;
			n2 = pad.count - 1 - index;
			break;
		case Footprint::Numbering::ZIGZAG:
			//n1 = pad.number + (index * 2) * pad.increment;
			//n2 = pad.number + (index * 2 + 1) * pad.increment;
			n1 = index * 2;
			n2 = index * 2 + 1;
			break;
		case Footprint::Numbering::DOUBLE:
			//n1 = pad.number + index * pad.increment;
			//n2 = n1;
			n1 = index;
			n2 = n1;
			break;
		}

		// first row
		if (pad.exists(n1)) {
			generatePad(s, pad.getName(n1), position1, pad.size, pad.shape, pad.drillSize, drillOffset1, pad.clearance, pad.maskMargin, pad.back);
			addSilkscreenPad(clips, position1, pad.size, pad.drillSize);
		}

		// second row
		if (pad.exists(n2)) {
			generatePad(s, pad.getName(n2), position2, pad.size, pad.shape, pad.drillSize, drillOffset2, pad.clearance, pad.maskMargin, pad.back);
			addSilkscreenPad(clips, position2, pad.size, pad.drillSize);
		}

		// increment position
		position1.x += pad.pitch;
		position2.x += pad.pitch;
	}
}

void generateQuad(std::ofstream &s, double2 globalPosition, const Footprint::Pad &pad, clipper2::Paths64 &clips) {

}

void generateMatrix(std::ofstream &s, double2 globalPosition, const Footprint::Pad &pad, clipper2::Paths64 &clips) {

}

bool generateFootprint(const fs::path &path, const std::string &name, const Footprint &footprint) {
	auto bodySize =  footprint.body.xy();
	bool haveBody = bodySize.positive();
	double2 refPosition = {0, 0};
	double2 valuePosition = {0, 0};
	double maskMargin = 0;
	double pasteMargin = 0;

	std::ofstream s((path / (name + ".kicad_mod")).string());

	// header
	s << "(module " << name << " (layer F.Cu) (tedit 5EC043C1)" << std::endl;
	s << "  (descr \"" << footprint.description << "\")" << std::endl;
	s << "  (attr " << (footprint.getType() == Footprint::Type::THROUGH_HOLE ? "through_hole" : "smd") << ')' << std::endl;
	if (haveBody)
		s << "  (model \"" << name << ".wrl\" (at (xyz 0 0 0)) (scale (xyz 1 1 1)) (rotate (xyz 0 0 0)))" << std::endl;
	s << "  (fp_text reference REF** (at " << refPosition << ") (layer F.SilkS) (effects (font (size 1 1) (thickness 0.15))))" << std::endl;
	s << "  (fp_text value " << name << " (at " << valuePosition << ") (layer F.Fab) (effects (font (size 1 1) (thickness 0.15))))" << std::endl;
	s << "  (solder_mask_margin " << maskMargin << ")" << std::endl;
	s << "  (solder_paste_margin " << pasteMargin << ")" << std::endl;

	clipper2::Clipper64 clipper;
	clipper2::Paths64 clips;

	// body
	if (haveBody) {
		double2 size = bodySize + footprint.margin * 2.0;
		if (!footprint.pads.empty() && footprint.pads.front().mirror)
			size.x *= -1;

		// courtyard
		rectangle(s, footprint.position, size, 0.05, "F.CrtYd");

		// fabrication layer
		fabRectangle(s, footprint.position, size);

		// add silkscreen rectangle
		if (footprint.silkscreen) {
			addSilkscreenRectangle(clipper, footprint.position, size);
		}
	}

	// components
	for (auto &pad : footprint.pads) {
		switch (pad.type) {
		case Footprint::Pad::Type::SINGLE:
			generateSingle(s, footprint.position, pad, clips);
			break;
		case Footprint::Pad::Type::DUAL:
			generateDual(s, footprint.position, pad, clips);
			break;
		case Footprint::Pad::Type::QUAD:
			generateQuad(s, footprint.position, pad, clips);
			break;
		case Footprint::Pad::Type::MATRIX:
			generateMatrix(s, footprint.position, pad, clips);
			break;
		}
	}

	// silkscreen
	if (footprint.silkscreen && haveBody) {
		clipper.AddClip(clips);

		// subtract pads from silkscreen
		clipper2::Paths64 dummy;
		clipper2::Paths64 result;
		clipper.Execute(clipper2::ClipType::Difference, clipper2::FillRule::NonZero, dummy, result);
		silkscreenPaths(s, result);
	}

	// footer
	s << ")" << std::endl;

	s.close();

	// return true when vrml should be generated
	return haveBody;
}

void generateVrml(const fs::path &path, const std::string &name, const Footprint &footprint) {
	std::ofstream s((path / (name + ".wrl")).string());

	double3 center(0, 0, footprint.body.z * (0.5 / 2.54));
	double3 size = footprint.body * (0.5 / 2.54);

	// header
	s << R"vrml(#VRML V2.0 utf8
Shape {
	appearance Appearance {material DEF mat Material {
		ambientIntensity 0.293
		diffuseColor 0.148 0.145 0.145
		specularColor 0.18 0.168 0.16
		emissiveColor 0.0 0.0 0.0
		transparency 0.0
		shininess 0.35
		}
	}
}
Shape {
	geometry IndexedFaceSet {
		creaseAngle 0.50
		coordIndex [3,0,2,-1,3,1,0,-1,6,5,7,-1,6,4,5,-1,1,4,0,-1,1,5,4,-1,7,2,6,-1,7,3,2,-1,2,4,6,-1,2,0,4,-1,7,1,3,-1,7,5,1]
		coord Coordinate {point [)vrml";

	for (int i = 0; i < 8; ++i) {
		if (i != 0)
			s << ',';
		double3 p = center + size * double3(i & 1 ? 1.0 : -1.0, i & 2 ? 1.0 : -1.0, i & 4 ? 1.0 : -1.0);
		s << p;
	}

s << R"vrml(]}
	}
	appearance Appearance {material USE mat}
}
)vrml";

	s.close();
}

int main(int argc, const char **argv) {
	if (argc < 2)
		return 1;
	fs::path path = argv[1];
	//fs::path path = "footprints.json";

	// read footprints
	std::map<std::string, Footprint> footprints;
	readJson(path, footprints);

	// generate footprints
	for (const auto &[name, footprint] : footprints) {
		// check if footprint is a template
		if (footprint.template_)
			continue;;
		std::cout << name << std::endl;

		auto dir = path.parent_path();
		if (generateFootprint(dir, name, footprint))
			generateVrml(dir, name, footprint);
	}

	return 0;
}
