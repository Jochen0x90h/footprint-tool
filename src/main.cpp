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

    // pad or pad array
    struct Pad {
        enum class Type {
            // single line of pads
            SINGLE,

            // dual pad lines (circular numbering)
            DUAL,

            // dual pad lines with zigzag numbering
            //ZIGZAG,

            // quad pad lines (square or rectangular)
            QUAD,

            // matrix of pads
            GRID
        };

        enum class Numbering {
            // number circular (counter clock wise)
            CIRCULAR,

            // number column-wise
            COLUMNS,

            // number row-wise
            ROWS,
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

        // double numbering
        bool double_ = false;

        // number of first pad
        int number = 1;

        // pad number increment
        int increment = 1;

        // pad names (override numbers)
        std::vector<std::string> names;

        // check if pin exists (pin with empty name does not exist)
        bool exists(int index) const {
            return index >= this->names.size() || !this->names[index].empty();
        }

        // get name of pin
        std::string getName(int index) const {
            if (index >= this->names.size()) {
                return std::to_string(this->number + index * this->increment);
            } else {
                return this->names[index];
            }
        }
    };

    // line or polyline
    struct Line {
        std::string layer;
        double width;
        std::vector<double2> points;
    };

    struct Circle {
        std::string layer;
        double width;
        double2 center;
        double radius;
    };


    // true if this is a template, i.e. no footprint gets generated
    bool template_ = false;

    // name and description of footprint
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

    // offset of body
    double3 offset;

    // list of pads (pad arrays)
    std::vector<Pad> pads;


    // shapes
    std::vector<Line> lines;
    std::vector<Circle> circles;


    // get type of footrpint
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
    else if (type == "grid")
        pad.type = Footprint::Pad::Type::GRID;

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
    if (numbering == "columns")
        pad.numbering = Footprint::Pad::Numbering::COLUMNS;
    else if (numbering == "rows")
        pad.numbering = Footprint::Pad::Numbering::ROWS;

    // double
    read(j, "double", pad.double_);

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
}

void readLine(json &j, Footprint::Line &line) {
    // layer
    read(j, "layer", line.layer);

    // width
    read(j, "width", line.width);

    // pads or pad arrays
    if (j.contains("points")) {
        auto jp = j.at("points");

        int size = jp.size();
        for (int i = 0; i < size - 1; i += 2) {
            double x = jp.at(i + 0).get<double>();
            double y = jp.at(i + 1).get<double>();
            line.points.emplace_back(x, y);
        }
    }
}

void readCircle(json &j, Footprint::Circle &circle) {
    // layer
    read(j, "layer", circle.layer);

    // width
    read(j, "width", circle.width);

    // center
    read(j, "center", circle.center);

    // diameter
    double diameter;
    read(j, "diameter", diameter);
    circle.radius = diameter * 0.5;
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

    // offset, applies only to body
    read(j, "offset", footprint.offset);

    // pads or pad arrays
    if (j.contains("pads")) {
        auto jp = j.at("pads");

        int size = jp.size();
        footprint.pads.resize(size);
        for (int i = 0; i < size; ++i) {
            // read pad or pad array
            readPad(jp.at(i), footprint.pads[i]);
        }
    } else {
        // no pads
        footprint.pads.clear();
    }

    // lines or polylines
    if (j.contains("lines")) {
        auto jl = j.at("lines");

        int count = jl.size();
        footprint.lines.resize(count);
        for (int i = 0; i < count; ++i) {
            // read line or polyline
            readLine(jl.at(i), footprint.lines[i]);
        }
    }

    // circles
    if (j.contains("circles")) {
        auto jc = j.at("circles");

        int count = jc.size();
        footprint.circles.resize(count);
        for (int i = 0; i < count; ++i) {
            // read circle
            readCircle(jc.at(i), footprint.circles[i]);
        }
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
    std::ifstream s(path.string());
    if (s.is_open()) {
        try {
            json j = json::parse(s,
                nullptr, // callback
                true, // allow exceptions
                true); // ignore comments

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
void writePad(std::ostream &s, std::string_view name, double2 position, double2 size, double shape, double2 drillSize, double2 padOffset, double clearance, double maskMargin, bool back) {
    bool hasPad = size.positive();
    bool hasDrill = drillSize.positive();

    // pad
    if (hasPad) {
        s << "  (pad \"" << name << "\" ";
        s << (hasDrill ? "thru_hole" : "smd");
    } else {
        // only hole
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
        if (!padOffset.zero())
            s << " (offset " << padOffset << ")";
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

void writeLine(std::ostream &s, double2 p1, double2 p2, double width, std::string_view layer) {
    s << "  (fp_line"
        " (start " << p1 << ")"
        " (end " << p2 << ")"
        " (stroke (width " << width << ") (type solid))"
        " (layer " << layer << ")"
        ")" << std::endl;
}

// draw a rectangle to courtyard layer
void writeRectangle(std::ostream &s, double2 center, double2 size, double width, std::string_view layer) {
    double x1 = center.x - size.x * 0.5;
    double y1 = center.y - size.y * 0.5;
    double x2 = center.x + size.x * 0.5;
    double y2 = center.y + size.y * 0.5;
    writeLine(s, {x1, y1}, {x2, y1}, width, layer);
    writeLine(s, {x2, y1}, {x2, y2}, width, layer);
    writeLine(s, {x2, y2}, {x1, y2}, width, layer);
    writeLine(s, {x1, y2}, {x1, y1}, width, layer);
}

// write line consisting of multiple segments
void writeLine(std::ostream &s, double2 position, const Footprint::Line &line) {
    int segmentCount = line.points.size() - 1;
    for (int i = 0; i < segmentCount; ++i) {
        auto p1 = position + line.points[i];
        auto p2 = position + line.points[i + 1];

        s << "  (fp_line"
            " (start " << p1 << ")"
            " (end " << p2 << ")"
            " (stroke (width " << line.width << ") (type solid))"
            " (layer \"" << line.layer << "\")"
            ")" << std::endl;
    }
}

// write circle
void writeCircle(std::ostream &s, double2 position, const Footprint::Circle &circle) {
    auto p1 = position + circle.center;
    auto p2 = p1 - double2(circle.radius, 0);
    s << "  (fp_circle"
        " (center " << p1 << ")"
        " (end " << p2 << ")"
        " (stroke (width " << circle.width << ") (type default))"
        " (fill none)"
        " (layer \"" << circle.layer << "\")"
        ")" << std::endl;
}

//(fp_circle (center -3 -3) (end -1 -3)
//    (stroke (width 0.1) (type default)) (fill none) (layer "Dwgs.User") (tstamp b059da20-dda9-4f2a-ae1a-8a4282f97b48))


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

void writeSilkscreenPaths(std::ostream &s, const clipper2::Paths64 &paths) {
    for (auto &path : paths) {
        int count = path.size();
        for (int i = 0; i < count - 1; ++i) {
            auto p1 = toPoint(path[i]);
            auto p2 = toPoint(path[(i + 1) % count]);
            writeLine(s, p1, p2, silkscreenWidth, "F.SilkS");
        }
    }
}

constexpr double fabWidth = 0.15;
constexpr double fabDistance = 0.2;

void writeFabRectangle(std::ofstream &s, double2 center, double2 size) {
    double x1 = center.x - size.x * 0.5;
    double y1 = center.y + size.y * 0.5;
    double x2 = center.x + size.x * 0.5;
    double y2 = center.y - size.y * 0.5;

    double d = std::min(std::abs(size.x), std::abs(size.y)) * 0.25;
    double x = x1 + (x2 > x1 ? d : -d);
    double y = y1 + (y2 > y1 ? d : -d);

    writeLine(s, {x, y1}, {x1, y}, silkscreenWidth, "F.Fab");
    writeLine(s, {x, y1}, {x2, y1}, silkscreenWidth, "F.Fab");
    writeLine(s, {x2, y1}, {x2, y2}, silkscreenWidth, "F.Fab");
    writeLine(s, {x2, y2}, {x1, y2}, silkscreenWidth, "F.Fab");
    writeLine(s, {x1, y2}, {x1, y}, silkscreenWidth, "F.Fab");
}

void writeSingle(std::ofstream &s, double2 globalPosition, const Footprint::Pad &pad, clipper2::Paths64 &clips) {
    int count = pad.count;
    bool hasPad = pad.size.positive();
    bool hasDrill = pad.drillSize.positive();

    // position of first pin
    double2 position = globalPosition + pad.position + double2((pad.pitch * (count - 1)) * -0.5, 0);

    // offset of pad relative to drill
    double2 padOffset = {0, 0};

    if (!hasDrill) {
        position += pad.offset;
    } else {
        position += pad.drillOffset;
        if (hasPad)
            padOffset = pad.offset - pad.drillOffset;
    }

    // generate pins
    for (int i = 0; i < count; ++i) {
        int index = pad.mirror ? count - 1 - i : i;

        int n = index;
        if (pad.double_) {
            // double pins
            n /= 2;
        }

        if (pad.exists(n)) {
            writePad(s, pad.getName(n), position, pad.size, pad.shape, pad.drillSize, padOffset, pad.clearance, pad.maskMargin, pad.back);
            addSilkscreenPad(clips, position, pad.size, pad.drillSize);
        }
        position.x += pad.pitch;
    }
}

void writeDual(std::ofstream &s, double2 globalPosition, const Footprint::Pad &pad, clipper2::Paths64 &clips) {
    int count = pad.count / 2;
    bool hasPad = pad.size.positive();
    bool hasDrill = pad.drillSize.positive();

    double padDistance = pad.distance.x;

    // position of first pin in each row
    double2 position1 = globalPosition + pad.position + double2((pad.pitch * (count - 1)) * -0.5, padDistance * 0.5);
    double2 position2 = globalPosition + pad.position + double2((pad.pitch * (count - 1)) * -0.5, padDistance * -0.5);

    // offset of pad relative to drill
    double2 padOffset1 = {0, 0};
    double2 padOffset2 = {0, 0};

    if (!hasDrill) {
        position1 += pad.offset;
        position2 -= pad.offset;
    } else {
        position1 += pad.drillOffset;
        position2 -= pad.drillOffset;
        if (hasPad) {
            padOffset1 = pad.offset - pad.drillOffset;
            padOffset2 = -padOffset1;
        }
    }

    // generate pins
    for (int i = 0; i < count; ++i) {
        int index = pad.mirror ? count - 1 - i : i;

        int n1, n2;
        if (pad.numbering == Footprint::Pad::Numbering::CIRCULAR) {
            // circular numbering
            n1 = index;
            n2 = pad.count - 1 - index;
        } else if (pad.numbering == Footprint::Pad::Numbering::COLUMNS) {
            // number by columns (zigzag)
            n1 = index * 2;
            n2 = index * 2 + 1;
        } else {
            // number by rows
            n1 = index;
            n2 = pad.count / 2 + index;
        }
        if (pad.double_) {
            // double pins
            n1 /= 2;
            n2 /= 2;
        }

        // first row
        if (pad.exists(n1)) {
            writePad(s, pad.getName(n1), position1, pad.size, pad.shape, pad.drillSize, padOffset1, pad.clearance, pad.maskMargin, pad.back);
            addSilkscreenPad(clips, position1, pad.size, pad.drillSize);
        }

        // second row
        if (pad.exists(n2)) {
            writePad(s, pad.getName(n2), position2, pad.size, pad.shape, pad.drillSize, padOffset2, pad.clearance, pad.maskMargin, pad.back);
            addSilkscreenPad(clips, position2, pad.size, pad.drillSize);
        }

        // increment position
        position1.x += pad.pitch;
        position2.x += pad.pitch;
    }
}

double2 rot90(double2 p) {
    return {p.y, p.x};
}

double2 swap(double2 p) {
    return {p.y, p.x};
}

// write quad (e.g. QFP)
void writeQuad(std::ofstream &s, double2 globalPosition, const Footprint::Pad &pad, clipper2::Paths64 &clips) {
    int count = pad.count / 4;
    bool hasPad = pad.size.positive();
    bool hasDrill = pad.drillSize.positive();

    // position of first pin in each row
    double2 position1 = globalPosition + pad.position + double2((pad.pitch * (count - 1)) * -0.5, pad.distance.x * 0.5);
    double2 position2 = globalPosition + pad.position + double2(pad.distance.y * 0.5, (pad.pitch * (count - 1)) * 0.5);
    double2 position3 = globalPosition + pad.position + double2((pad.pitch * (count - 1)) * 0.5, pad.distance.x * -0.5);
    double2 position4 = globalPosition + pad.position + double2(pad.distance.y * -0.5, (pad.pitch * (count - 1)) * -0.5);

    // offset of pad relative to drill
    double2 padOffset1 = {0, 0};
    double2 padOffset2 = {0, 0};
    double2 padOffset3 = {0, 0};
    double2 padOffset4 = {0, 0};

    if (!hasDrill) {
        position1 += pad.offset;
        position2 += rot90(pad.offset);
        position3 -= pad.offset;
        position4 -= rot90(pad.offset);
    } else {
        position1 += pad.drillOffset;
        position2 += rot90(pad.drillOffset);
        position3 -= pad.drillOffset;
        position4 -= rot90(pad.drillOffset);
        if (hasPad) {
            padOffset1 = pad.offset - pad.drillOffset;
            padOffset2 = rot90(pad.offset - pad.drillOffset);
            padOffset3 = -padOffset1;
            padOffset4 = -padOffset3;
        }
    }

    double2 padSize24 = swap(pad.size);

    // generate pins
    for (int i = 0; i < count; ++i) {
        int index = pad.mirror ? count - 1 - i : i;

        int n1 = index;
        int n2 = count + index;
        int n3 = count * 2 + index;
        int n4 = count * 3 + index;

        if (pad.exists(n1)) {
            writePad(s, pad.getName(n1), position1, pad.size, pad.shape, pad.drillSize, padOffset1, pad.clearance, pad.maskMargin, pad.back);
            addSilkscreenPad(clips, position1, pad.size, pad.drillSize);
        }
        if (pad.exists(n2)) {
            writePad(s, pad.getName(n2), position2, padSize24, pad.shape, swap(pad.drillSize), padOffset2, pad.clearance, pad.maskMargin, pad.back);
            addSilkscreenPad(clips, position2, padSize24, pad.drillSize);
        }
        if (pad.exists(n3)) {
            writePad(s, pad.getName(n3), position3, pad.size, pad.shape, pad.drillSize, padOffset3, pad.clearance, pad.maskMargin, pad.back);
            addSilkscreenPad(clips, position3, pad.size, pad.drillSize);
        }
        if (pad.exists(n4)) {
            writePad(s, pad.getName(n4), position4, padSize24, pad.shape, swap(pad.drillSize), padOffset4, pad.clearance, pad.maskMargin, pad.back);
            addSilkscreenPad(clips, position4, padSize24, pad.drillSize);
        }

        // increment position
        position1.x += pad.pitch;
        position2.y -= pad.pitch;
        position3.x -= pad.pitch;
        position4.y += pad.pitch;
    }

}

// generate grid (e.g. BGA)
void writeGrid(std::ofstream &s, double2 globalPosition, const Footprint::Pad &pad, clipper2::Paths64 &clips) {

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
        double2 position = footprint.position + footprint.offset.xy();
        double2 silkscreenSize = bodySize + footprint.margin * 2.0;

        // apply mirror to size so that pin1 marker is placed at the rigt position
        if (!footprint.pads.empty() && footprint.pads.front().mirror) {
            bodySize.x *= -1;
            silkscreenSize.x *= -1;
        }

        // courtyard
        writeRectangle(s, position, bodySize, 0.05, "F.CrtYd");

        // fabrication layer
        writeFabRectangle(s, position, bodySize);

        // add silkscreen rectangle
        if (footprint.silkscreen) {
            addSilkscreenRectangle(clipper, position, silkscreenSize);
        }
    }

    // pads
    for (auto &pad : footprint.pads) {
        switch (pad.type) {
        case Footprint::Pad::Type::SINGLE:
            writeSingle(s, footprint.position, pad, clips);
            break;
        case Footprint::Pad::Type::DUAL:
            writeDual(s, footprint.position, pad, clips);
            break;
        case Footprint::Pad::Type::QUAD:
            writeQuad(s, footprint.position, pad, clips);
            break;
        case Footprint::Pad::Type::GRID:
            writeGrid(s, footprint.position, pad, clips);
            break;
        }
    }

    // lines
    for (auto &line : footprint.lines) {
        writeLine(s, footprint.position, line);
    }

    // circles
    for (auto &circle : footprint.circles) {
        writeCircle(s, footprint.position, circle);
    }

    // silkscreen
    if (footprint.silkscreen && haveBody) {
        clipper.AddClip(clips);

        // subtract pads from silkscreen
        clipper2::Paths64 dummy;
        clipper2::Paths64 result;
        clipper.Execute(clipper2::ClipType::Difference, clipper2::FillRule::NonZero, dummy, result);
        writeSilkscreenPaths(s, result);
    }

    // footer
    s << ")" << std::endl;

    s.close();

    // return true when vrml should be generated
    return haveBody;
}

// generate a box as vrml as minimalistic 3D visualization
void generateVrml(const fs::path &path, const std::string &name, const Footprint &footprint) {
    std::ofstream s((path / (name + ".wrl")).string());

    // center of box
    double3 center = footprint.offset + double3(footprint.position.x, footprint.position.y, 0);
    center.y = -center.y;

    // size of box
    double3 size = footprint.body;

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
        double3 p = (center + size * double3(i & 1 ? 0.5 : -0.5, i & 2 ? 0.5 : -0.5, i & 4 ? 1.0 : 0.0)) / 2.54;
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
