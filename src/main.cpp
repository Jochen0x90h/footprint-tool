#include "clipper2.hpp"
#include "double3.hpp"
#include "Footprint.hpp"
#include "generateStep.hpp"
#include "generateVrml.hpp"
#include <nlohmann/json.hpp>
//#include <uuid.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <numbers>
#include <set>


#define VERSION "0.3.0"


using namespace std::numbers;
using json = nlohmann::json;
namespace fs = std::filesystem;


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

void read(json &j, const std::string &key, double2 &value) {
    if (j.contains(key)) {
        json jv = j.at(key);
        value.x = jv.at(0).get<double>();
        value.y = jv.at(1).get<double>();
    }
}

void readRelaxed(json &j, const std::string &key, double2 &value) {
    if (j.contains(key)) {
        json jv = j.at(key);
        if (jv.is_number()) {
            // scalar value, use for both x and y
            value.x = jv.get<double>();
            value.y = value.x;
        } else if (jv.is_array()) {
            // array of values, use first two values for x and y
            value.x = jv.at(0).get<double>();
            if (jv.size() >= 2)
                value.y = jv.at(1).get<double>();
            else
                value.y = value.x;
        }
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

void read2or3(json &j, const std::string &key, double3 &value) {
    if (j.contains(key)) {
        json jv = j.at(key);

        // array of values, use first two values for x and y
        value.x = jv.at(0).get<double>();
        value.y = jv.at(1).get<double>();
        if (jv.size() >= 3)
            value.z = jv.at(2).get<double>();
        else
            value.z = 0;
    }
}

void readPad(json &j, Footprint::Pad &pad) {
    // type
    std::string type = j.value("type", std::string());
    if (type == "dual")
        pad.type = Footprint::Pad::Type::DUAL;
    else if (type == "quad")
        pad.type = Footprint::Pad::Type::QUAD;
    else if (type == "grid")
        pad.type = Footprint::Pad::Type::GRID;

    // position
    read(j, "position", pad.position);

    // distance
    readRelaxed(j, "distance", pad.distance);

    // pitch
    read(j, "pitch", pad.pitch);

    // shift
    read(j, "shift", pad.shift);

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

    // layers
    read(j, "back", pad.back);
    /*read(j, "jumper", pad.jumper);
    if (pad.jumper) {
        // set defaults for jumper
        pad.mask = false;
        pad.paste = false;
    }*/
    read(j, "mask", pad.mask);
    read(j, "paste", pad.paste);

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
        pad.count = std::max(pad.count, int(pad.names.size()));
    }
}

void readLine(json &j, Footprint::Line &line) {
    // layer
    read(j, "layer", line.layer);

    // width
    read(j, "width", line.width);

    // list of points
    if (j.contains("points")) {
        auto jp = j.at("points");

        int size = jp.size();
        for (int i = 0; i < size - 1; i += 2) {
            double x = jp.at(i + 0).get<double>();
            double y = jp.at(i + 1).get<double>();
            line.points.emplace_back(x, y);
        }
    }

    // joint radius
    read(j, "radius", line.radius);
}

void readRectangle(json &j, Footprint::Rectangle &rectangle) {
    // layer
    read(j, "layer", rectangle.layer);

    // width
    read(j, "width", rectangle.width);

    // p1
    read(j, "p1", rectangle.p1);

    // p2
    read(j, "p2", rectangle.p2);

    // radius
    read(j, "radius", rectangle.radius);
}

void readCircle(json &j, Footprint::Circle &circle) {
    // layer
    read(j, "layer", circle.layer);

    // fill
    read(j, "fill", circle.fill);
    if (circle.fill) {
        // set default width for filled circle
        circle.width = 0;
    }

    // width
    read(j, "width", circle.width);


    // center
    read(j, "center", circle.center);

    // diameter or radius
    double diameter;
    read(j, "diameter", diameter);
    circle.radius = diameter * 0.5;
    read(j, "radius", circle.radius);
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

    // global position, applies to everything
    read(j, "position", footprint.position);

    // body
    if (j.contains("body")) {
        auto b = j.at("body");
        auto &body = footprint.body;
        read(b, "size", body.size);
        read2or3(b, "offset", body.offset);
    }

    // silkscreen
    if (j.contains("silkscreen")) {
        auto s = j.at("silkscreen");
        auto &silkscreen = footprint.silkscreen;//.emplace();

        read(s, "enabled", silkscreen.enabled);

        std::string marker = s.value("marker", std::string());
        if (marker == "none")
            silkscreen.marker = Footprint::Silkscreen::Marker::NONE;
        else if (marker == "bar")
            silkscreen.marker = Footprint::Silkscreen::Marker::BAR;

        readRelaxed(s, "margin", silkscreen.margin);
        read(s, "offset", silkscreen.offset);
    }

    // courtyard
    if (j.contains("courtyard")) {
        auto c = j.at("courtyard");
        auto &courtyard = footprint.courtyard;

        read(c, "enabled", courtyard.enabled);
        readRelaxed(c, "margin", courtyard.margin);
        read(c, "offset", courtyard.offset);
    }

    // fabrication layer
    if (j.contains("fabrication")) {
        auto f = j.at("fabrication");
        auto &fabrication = footprint.fabrication;

        read(f, "enabled", fabrication.enabled);
        readRelaxed(f, "margin", fabrication.margin);
        read(f, "offset", fabrication.offset);
    }

    // orientation (position of pin 1 marker)
    std::string orientation = j.value("orientation", std::string());
    if (orientation == "top-left")
        footprint.orientation = Footprint::Orientation::TOP_LEFT;
    else if (orientation == "bottom-right")
        footprint.orientation = Footprint::Orientation::BOTTOM_RIGHT;
    else if (orientation == "top-right")
        footprint.orientation = Footprint::Orientation::TOP_RIGHT;

    // jumper meta-property sets some defaults
    bool jumper = false;
    read(j, "jumper", jumper);
    if (jumper) {
        footprint.excludeFromBom = true;
        footprint.excludeFromPosFiles = true;
        footprint.allowSolderMaskBridges = true;
    }


    // pads or pad arrays
    if (j.contains("pads")) {
        auto jp = j.at("pads");

        int size = jp.size();
        footprint.pads.resize(size);
        for (int i = 0; i < size; ++i) {
            auto &pad = footprint.pads[i];

            // set defaults for jumper
            if (jumper) {
                pad.mask = false;
                pad.paste = false;
            }

            // read pad or pad array
            readPad(jp.at(i), pad);
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

    // rectangles
    if (j.contains("rectangles")) {
        auto jr = j.at("rectangles");

        int count = jr.size();
        footprint.rectangles.resize(count);
        for (int i = 0; i < count; ++i) {
            // read rectangle
            readRectangle(jr.at(i), footprint.rectangles[i]);
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

            bool versionFound = false;
            for (auto& [name, value] : j.items()) {
                Footprint footprint;

                try {
                    if (name == "version") {
                        // get version
                        versionFound = true;
                        std::string version = value.get<std::string>();
                        if (version != VERSION)
                            std::cerr << "warning: version mismatch, expected " VERSION " but got " << version << std::endl;
                    } else {
                        readFootprint(value, footprints, footprint);
                        footprints[name] = footprint;
                    }
                } catch (std::exception &e) {
                    // parsing the json file failed
                    std::cerr << name << ": " << e.what() << std::endl;
                }
            }
            if (!versionFound)
                std::cerr << "warning: version not specified, expected " VERSION << std::endl;

        } catch (std::exception &e) {
            // parsing the json file failed
            std::cerr << "json: " << e.what() << std::endl;
        }
    } else {
        std::cerr << "error: could not open file " << path.string() << std::endl;
    }
}

/*
// UUIDs are optional for footprints, therefore omit
namespace uuids {
    inline uuid_random_generator& get_uuid_generator() {
        static std::mt19937 engine = []() {
            // generate seed
            std::random_device rd;
            auto seed_data = std::array<int, std::mt19937::state_size>{};
            std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
            std::seed_seq seq(std::begin(seed_data), std::end(seed_data));

            // create engine
            return std::mt19937{seq};
        }();

        static uuid_random_generator generator{engine};

        return generator;
    }
}

// Globale Funktion – einfach zu benutzen
inline uuids::uuid generateUuid() {
    return uuids::get_uuid_generator()();
}
*/

// define a pad
void writePad(std::ostream &s, std::string_view name, double2 position, double2 size, double2 offset, double shape,
    double2 drillSize, const Footprint::Pad &pad)
{
    bool hasPad = size.positive();
    bool hasDrill = drillSize.positive();

    // pad
    if (hasPad) {
        // through-hole or smd
        s << "  (pad \"" << name << "\" ";
        s << (hasDrill ? "thru_hole" : "smd");
    } else {
        // only hole
        s << "  (pad \"\" np_thru_hole";
        shape = CIRCLE;
        size = drillSize;
    }

    // shape
    if (shape <= RECTANGLE)
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
        if (!offset.zero())
            s << " (offset " << offset << ")";
        s << ")";
    }

    // margins
    if (pad.clearance > 0)
        s << " (clearance " << pad.clearance << ")";
    if (pad.maskMargin != 0)
        s << " (solder_mask_margin " << pad.maskMargin << ")";

    // layers
    s << " (layers";
    if (hasDrill) {
        // front and back
        s << " \"*.Cu\"";
        if (pad.mask)
            s << " \"*.Mask\"";
    } else if (!pad.back) {
        // front
        s << " \"F.Cu\"";
        if (pad.mask)
            s << " \"F.Mask\"";
        if (pad.paste)
            s << " \"F.Paste\"";
    } else {
        // back
        s << " \"B.Cu\"";
        if (pad.mask)
            s << " \"B.Mask\"";
        if (pad.paste)
            s << " \"B.Paste\"";
    }
    s << ")";
    if (hasPad && hasDrill)
        s << " (remove_unused_layers) (keep_end_layers)";

    // uuid
    //s << " (uuid \"" << generateUuid() << "\")";

    // closing brace
    s << ')' << std::endl;
}

// write a single line
void writeLine(std::ostream &s, double2 p1, double2 p2, double width, std::string_view layer) {
    s << "  (fp_line"
        " (start " << p1 << ")"
        " (end " << p2 << ")"
        " (stroke (width " << width << ") (type solid))"
        " (layer " << layer << ")"
        //" (uuid \"" << generateUuid() << "\")"
        ")" << std::endl;
}

// write line consisting of multiple segments
/*void writeLine(std::ostream &s, double2 position, const Footprint::Line &line) {
    int segmentCount = line.points.size() - 1;
    for (int i = 0; i < segmentCount; ++i) {
        auto p1 = position + line.points[i];
        auto p2 = position + line.points[i + 1];

        writeLine(s, p1, p2, line.width, line.layer);
    }
}*/

void writeLine(std::ostream &s, double2 position, const Footprint::Line &line) {
    int segmentCount = line.points.size() - 1;
    if (line.radius <= 0) {
        for (int i = 0; i < segmentCount; ++i) {
            auto p1 = position + line.points[i];
            auto p2 = position + line.points[i + 1];

            writeLine(s, p1, p2, line.width, line.layer);
        }
    } else {
        auto p1 = position + line.points[0];
        auto p2 = position + line.points[1];
        for (int i = 2; i <= segmentCount; ++i) {
            auto p3 = position + line.points[i];


            auto d1 = p2 - p1;
            auto d2 = p3 - p2;

            // unit directions
            auto u1 = normalize(d1);
            auto u2 = normalize(d2);

            // unit normals
            double2 n1 = {-u1.y, u1.x};
            double2 n2 = {-u2.y, u2.x};

            double cross = n1.x * n2.y - n1.y * n2.x;

            double angle = pi - asin(cross);

            // flip normals if winding direction is negative
            if (cross < 0) {
                n1 = -n1;
                n2 = -n2;
            }

            // direction of circle center
            auto d = normalize(n1 + n2);

            // circle center
            auto c = p2 + d * (line.radius / sin(angle * 0.5));

            // segment angles
            double a1 = atan2(-n1.y, -n1.x);
            double a2 = atan2(-n2.y, -n2.x);
            if (a2 - a1 > pi)
                a2 -= 2 * pi;
            if (a2 - a1 < -pi)
                a2 += 2 * pi;

            // number of circle steps
            int count = int(abs(a2 - a1) * 30 / pi) + 1;

            // write circle segment
            double aStep = (a2 - a1) / count;
            for (int i = 0; i <= count; ++i) {
                double a = a1 + i * aStep;
                auto p = c + double2{cos(a), sin(a)};

                writeLine(s, p1, p, line.width, line.layer);
                p1 = p;
            }

            // next
            p2 = p3;
        }
        writeLine(s, p1, p2, line.width, line.layer);
    }
}

// write rectangle
void writeRectangle(std::ostream &s, double2 position, const Footprint::Rectangle &rectangle) {
    auto x1 = position + rectangle.p1;
    auto x2 = position + rectangle.p2;
    auto p1 = min(x1, x2);
    auto p2 = max(x1, x2);
    s << "  (fp_rect"
        " (start " << p1 << ")"
        " (end " << p2 << ")";
    if (rectangle.radius > 0)
        s << " (radius " << rectangle.radius << ")";
    s << " (stroke (width " << rectangle.width << ") (type solid))"
        " (fill no)"
        " (layer \"" << rectangle.layer << "\")"
        //" (uuid \"" << generateUuid() << "\")"
        ")" << std::endl;
}

// write circle
void writeCircle(std::ostream &s, double2 position, const Footprint::Circle &circle) {
    auto p1 = position + circle.center;
    auto p2 = p1 - double2(circle.radius, 0);
    s << "  (fp_circle"
        " (center " << p1 << ")"
        " (end " << p2 << ")"
        " (stroke (width " << circle.width << ") (type default))"
        " (fill " << (circle.fill ? "solid" : "none") << ")"
        " (layer \"" << circle.layer << "\")"
        //" (uuid \"" << generateUuid() << "\")"
        ")" << std::endl;
}

//(fp_circle (center -3 -3) (end -1 -3)
//    (stroke (width 0.1) (type default)) (fill none) (layer "Dwgs.User") (tstamp b059da20-dda9-4f2a-ae1a-8a4282f97b48))

// draw a rectangle to the given layer
void writeRectangle(std::ostream &s, double2 center, double2 size, double width, std::string_view layer) {
    double w = size.x;
    double h = size.y;

    double x1 = center.x - w * 0.5;
    double y1 = center.y - h * 0.5;
    double x2 = center.x + w * 0.5;
    double y2 = center.y + h * 0.5;
    writeLine(s, {x1, y1}, {x2, y1}, width, layer);
    writeLine(s, {x2, y1}, {x2, y2}, width, layer);
    writeLine(s, {x2, y2}, {x1, y2}, width, layer);
    writeLine(s, {x1, y2}, {x1, y1}, width, layer);
}


double2 orient(double x, double y, Footprint::Orientation o) {
    switch (o) {
    default: // BOTTOM_LEFT
        return { x,  y};
    case Footprint::Orientation::BOTTOM_RIGHT:
        return { y, -x};
    case Footprint::Orientation::TOP_LEFT:
        return {-y,  x};
    case Footprint::Orientation::TOP_RIGHT:
        return {-x, -y};
    }
}

// line widht for silkscreen
constexpr double silkscreenWidth = 0.15;
constexpr double silkscreenDistance = 0.1;
constexpr double padClearance = 0.1;

// add a rectangle pith pin 1 indicator to silscreen clipper
void addSilkscreenRectangle(clipper2::Clipper64 &clipper, double2 center, double2 size, Footprint::Orientation o,
    Footprint::Silkscreen::Marker marker)
{
    double w = size.x;
    double h = size.y;
    if (o == Footprint::Orientation::BOTTOM_RIGHT || o == Footprint::Orientation::TOP_LEFT) {
        // swap width and height
        w = size.y;
        h = size.x;
    }

    double x1 = - w * 0.5;
    double y1 = + h * 0.5;
    double x2 = + w * 0.5;
    double y2 = - h * 0.5;

    double d = 4 * silkscreenWidth;

    switch (marker) {
    case Footprint::Silkscreen::Marker::NONE:
        {
            clipper2::Paths64 paths;
            clipper2::Path64 &path = paths.emplace_back();
            path.push_back(toClipperPoint(center + orient(x1, y1, o)));
            path.push_back(toClipperPoint(center + orient(x2, y1, o)));
            path.push_back(toClipperPoint(center + orient(x2, y2, o)));
            path.push_back(toClipperPoint(center + orient(x1, y2, o)));
            path.push_back(toClipperPoint(center + orient(x1, y1, o)));
            clipper.AddOpenSubject(paths);
        }
        break;
    case Footprint::Silkscreen::Marker::DOT:
        {
            clipper2::Paths64 paths;
            clipper2::Path64 &path = paths.emplace_back();
            double x = x1 + (x2 > x1 ? d : -d);
            double y = y1 + (y2 > y1 ? d : -d);
            path.push_back(toClipperPoint(center + orient(x, y1, o)));
            path.push_back(toClipperPoint(center + orient(x2, y1, o)));
            path.push_back(toClipperPoint(center + orient(x2, y2, o)));
            path.push_back(toClipperPoint(center + orient(x1, y2, o)));
            path.push_back(toClipperPoint(center + orient(x1, y, o)));
            clipper.AddOpenSubject(paths);
        }

        // add pin1 indicator
        {
            clipper2::Paths64 paths;
            clipper2::Path64 &path = paths.emplace_back();
            double w = silkscreenWidth * 0.5;
            path.push_back(toClipperPoint(center + orient(x1 - w, y1 - w, o)));
            path.push_back(toClipperPoint(center + orient(x1 + w, y1 - w, o)));
            path.push_back(toClipperPoint(center + orient(x1 + w, y1 + w, o)));
            path.push_back(toClipperPoint(center + orient(x1 - w, y1 + w, o)));
            clipper.AddSubject(paths);
        }
        break;
    case Footprint::Silkscreen::Marker::BAR:
        {
            clipper2::Paths64 paths;
            clipper2::Path64 &path = paths.emplace_back();
            double x = x1 + (x2 > x1 ? d : -d);
            path.push_back(toClipperPoint(center + orient(x, y1, o)));
            path.push_back(toClipperPoint(center + orient(x2, y1, o)));
            path.push_back(toClipperPoint(center + orient(x2, y2, o)));
            path.push_back(toClipperPoint(center + orient(x, y2, o)));
            clipper.AddOpenSubject(paths);
        }

        // add pin1 indicator
        {
            clipper2::Paths64 paths;
            clipper2::Path64 &path = paths.emplace_back();
            double w = silkscreenWidth * 0.5;
            path.push_back(toClipperPoint(center + orient(x1 - w, y2 - w, o)));
            path.push_back(toClipperPoint(center + orient(x1 + w, y2 - w, o)));
            path.push_back(toClipperPoint(center + orient(x1 + w, y1 + w, o)));
            path.push_back(toClipperPoint(center + orient(x1 - w, y1 + w, o)));
            clipper.AddSubject(paths);
        }
        break;
    }
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

void writeSilkscreenPaths(std::ostream &s, const clipper2::Paths64 &paths, int open = 0) {
    for (auto &path : paths) {
        int count = path.size();
        for (int i = 0; i < count - open; ++i) {
            auto p1 = toPoint(path[i]);
            auto p2 = toPoint(path[(i + 1) % count]);
            writeLine(s, p1, p2, silkscreenWidth, "F.SilkS");
        }
    }
}

constexpr double fabWidth = 0.15;
constexpr double fabDistance = 0.2;

void writeFabRectangle(std::ofstream &s, double2 center, double2 size, Footprint::Orientation o) {
    double w = size.x;
    double h = size.y;
    if (o == Footprint::Orientation::BOTTOM_RIGHT || o == Footprint::Orientation::TOP_LEFT) {
        // swap width and height
        w = size.y;
        h = size.x;
    }

    double x1 = - w * 0.5;
    double y1 = + h * 0.5;
    double x2 = + w * 0.5;
    double y2 = - h * 0.5;

    double d = std::min(std::abs(size.x), std::abs(size.y)) * 0.25;
    double x = x1 + (x2 > x1 ? d : -d);
    double y = y1 + (y2 > y1 ? d : -d);

    writeLine(s, center + orient(x, y1, o), center + orient(x1, y, o), silkscreenWidth, "F.Fab");
    writeLine(s, center + orient(x, y1, o), center + orient(x2, y1, o), silkscreenWidth, "F.Fab");
    writeLine(s, center + orient(x2, y1, o), center + orient(x2, y2, o), silkscreenWidth, "F.Fab");
    writeLine(s, center + orient(x2, y2, o), center + orient(x1, y2, o), silkscreenWidth, "F.Fab");
    writeLine(s, center + orient(x1, y2, o), center + orient(x1, y, o), silkscreenWidth, "F.Fab");
}

void writeSingle(std::ofstream &s, const Footprint &footprint, const Footprint::Pad &pad, clipper2::Paths64 &clips) {
    int count = pad.count;
    bool hasPad = pad.size.positive();
    bool hasDrill = pad.drillSize.positive();

    // position of first pin
    double2 position = footprint.position + pad.position;// + double2((pad.pitch * (count - 1)) * -0.5, 0);

    // pitch
    double2 pitch = {0, 0};

    // offset of pad relative to drill
    double2 padOffset = {0, 0};

    double start = pad.pitch * (count - 1) * 0.5;

    // adjust position/pitch depending on orientation
    if (footprint.orientation == Footprint::Orientation::BOTTOM_LEFT) {
        // pin 1 marker is bottom left
        position += double2(-start, 0);

        // advance in positive x direction (horizontal)
        pitch.x = pad.pitch;
    } else if (footprint.orientation == Footprint::Orientation::BOTTOM_RIGHT) {
        // pin 1 marker is bottom right
        position += double2(0, start);

        // advance in negative y direction (vertical)
        pitch.y = -pad.pitch;
    } else if (footprint.orientation == Footprint::Orientation::TOP_LEFT) {
        // pin 1 marker is top left
        position += double2(0, -start);

        // advance in positive y direction (vertical)
        pitch.y = pad.pitch;
    } else {
        // pin 1 marker is top right
        position += double2(start, 0);

        // advance in negative x direction (horizontal)
        pitch.x = -pad.pitch;
    }

    // adjust position/offset depending on drill
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
            writePad(s, pad.getName(n), position, pad.size, padOffset, pad.shape, pad.drillSize, pad);
            addSilkscreenPad(clips, position, pad.size, pad.drillSize);
        }
        position += pitch;
    }
}

void writeDual(std::ofstream &s, const Footprint &footprint, const Footprint::Pad &pad,
    clipper2::Paths64 &clips)
{
    int count1 = (pad.count + 1) / 2;
    int count2 = pad.count / 2;
    bool hasPad = pad.size.positive();
    bool hasDrill = pad.drillSize.positive();

    double padDistance = pad.distance.x;

    // center position  of first pin in each row (start with center)
    double2 position = footprint.position + pad.position;

    // shift
    double shift = pad.shift;

    // offset of pad relative to drill
    double2 padOffset1 = {0, 0};
    double2 padOffset2 = {0, 0};

    double start1 = pad.pitch * (count1 - 1) * 0.5 + shift;
    double start2 = pad.pitch * (count2 - 1) * 0.5 - shift;

    // calc position/pitch depending on orientation
    double2 position1 = position;
    double2 position2 = position;
    double2 pitch = {0, 0};
    if (footprint.orientation == Footprint::Orientation::BOTTOM_LEFT) {
        // pin 1 marker is bottom left
        position1 += double2(-start1, padDistance * 0.5);
        position2 += double2(-start2, padDistance * -0.5);

        // advance in positive x direction (horizontal)
        pitch.x = pad.pitch;
    } else if (footprint.orientation == Footprint::Orientation::BOTTOM_RIGHT) {
        // pin 1 marker is bottom right
        position1 += double2(padDistance * 0.5, start1);
        position2 += double2(padDistance * -0.5, start2);

        // advance in negative y direction (vertical)
        pitch.y = -pad.pitch;
    } else if (footprint.orientation == Footprint::Orientation::TOP_LEFT) {
        // pin 1 marker is top left
        position1 += double2(padDistance * -0.5, -start1);
        position2 += double2(padDistance * 0.5, -start2);

        // advance in positive y direction (vertical)
        pitch.y = pad.pitch;
    } else {
        // pin 1 marker is top right
        position1 += double2(start1, padDistance * -0.5);
        position2 += double2(start2, padDistance * 0.5);

        // advance in negative x direction (horizontal)
        pitch.x = -pad.pitch;
    }

    // adjust position/offset depending on drill
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
    for (int i = 0; i < count1; ++i) {
        int index = pad.mirror ? count1 - 1 - i : i;

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
            writePad(s, pad.getName(n1), position1, pad.size, padOffset1, pad.shape, pad.drillSize, pad);
            addSilkscreenPad(clips, position1, pad.size, pad.drillSize);
        }

        // second row
        if (i < count2 && pad.exists(n2)) {
            writePad(s, pad.getName(n2), position2, pad.size, padOffset2, pad.shape, pad.drillSize, pad);
            addSilkscreenPad(clips, position2, pad.size, pad.drillSize);
        }

        // increment position
        position1 += pitch;
        position2 += pitch;
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
            writePad(s, pad.getName(n1), position1, pad.size, padOffset1, pad.shape, pad.drillSize, pad);
            addSilkscreenPad(clips, position1, pad.size, pad.drillSize);
        }
        if (pad.exists(n2)) {
            writePad(s, pad.getName(n2), position2, padSize24, padOffset2, pad.shape, swap(pad.drillSize), pad);
            addSilkscreenPad(clips, position2, padSize24, pad.drillSize);
        }
        if (pad.exists(n3)) {
            writePad(s, pad.getName(n3), position3, pad.size, padOffset3, pad.shape, pad.drillSize, pad);
            addSilkscreenPad(clips, position3, pad.size, pad.drillSize);
        }
        if (pad.exists(n4)) {
            writePad(s, pad.getName(n4), position4, padSize24, padOffset4, pad.shape, swap(pad.drillSize), pad);
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
    double2 position = footprint.position + footprint.body.offset.xy();

    double mirror = !footprint.pads.empty() && footprint.pads.front().mirror ? -1 : 1;

    // body
    auto bodySize = footprint.body.size.xy();
    bool haveBody = bodySize.positive();

    // silkscreen
    bool haveSilkscreen = haveBody && footprint.silkscreen.enabled;

    // courtyard
    bool haveCourtyard = haveBody && footprint.courtyard.enabled;

    // fabrication
    bool haveFabrication = haveBody && footprint.fabrication.enabled;

    double2 refPosition = {0, 0};
    double2 valuePosition = {0, 0};
    double maskMargin = 0;
    double pasteMargin = 0;


    std::ofstream s((path / (name + ".kicad_mod")).string());

    // header
    s << "(footprint \"" << name << "\"" << std::endl;
    s << "  (version 20260206)" << std::endl;
    s << "  (generator \"pcbnew\")" << std::endl;
	s << "  (generator_version \"10.0\")" << std::endl;
	s << "  (layer \"F.Cu\")" << std::endl;

    // description
    s << "  (descr \"" << footprint.description << "\")" << std::endl;

    // properties
    s << "  (property \"Reference\" \"REF**\"" << std::endl;
    s << "    (at 0 0 0)" << std::endl;
    s << "    (layer \"F.SilkS\")" << std::endl;
    //s << "    (uuid \"" << generateUuid() << "\")" << std::endl;
    s << "    (effects (font (size 1 1) (thickness 0.15)))" << std::endl;
    s << "  )" << std::endl;
    s << "  (property \"Value\" \"" << name << "\"" << std::endl;
    s << "    (at 0 0 0)" << std::endl;
    s << "    (layer \"F.Fab\")" << std::endl;
    //s << "    (uuid \"" << generateUuid() << "\")" << std::endl;
    s << "    (effects (font (size 1 1) (thickness 0.15)))" << std::endl;
    s << "  )" << std::endl;
    s << "  (property \"Datasheet\" \"\"" << std::endl;
    s << "    (at 0 0 0)" << std::endl;
    s << "    (layer \"F.Fab\")" << std::endl;
    s << "    (hide yes)" << std::endl;
    //s << "    (uuid \"" << generateUuid() << "\")" << std::endl;
    s << "    (effects (font (size 1.27 1.27)))" << std::endl;
    s << "  )" << std::endl;
    s << "  (property \"Description\" \"\"" << std::endl;
    s << "    (at 0 0 0)" << std::endl;
    s << "    (layer \"F.Fab\")" << std::endl;
    s << "    (hide yes)" << std::endl;
    //s << "    (uuid \"" << generateUuid() << "\")" << std::endl;
    s << "    (effects (font (size 1.27 1.27)))" << std::endl;
    s << "  )" << std::endl;

    // attributes
    s << "  (attr";
    s << (footprint.getType() == Footprint::Type::THROUGH_HOLE ? " through_hole" : " smd");
    if (footprint.excludeFromBom)
        s << " exclude_from_bom";
    if (footprint.excludeFromPosFiles)
        s << " exclude_from_pos_files";
    if (footprint.allowSolderMaskBridges)
        s << " allow_soldermask_bridges";
    s  << ')' << std::endl;
    s << "  (duplicate_pad_numbers_are_jumpers no)" << std::endl;

    // margins
    s << "  (solder_mask_margin " << maskMargin << ")" << std::endl;
    s << "  (solder_paste_margin " << pasteMargin << ")" << std::endl;

    // clipper for silkscreen
    clipper2::Clipper64 clipper;
    clipper2::Paths64 clips; // shapes that clip away the silkscreen, e.g. pads

    // silkscreen
    if (haveSilkscreen) {
        auto &silkscreen = footprint.silkscreen;
        double2 silkscreenSize = bodySize + silkscreen.margin * 2.0;
        silkscreenSize.x *= mirror;

        addSilkscreenRectangle(clipper, position + silkscreen.offset, silkscreenSize,
            footprint.orientation, silkscreen.marker);
    }

    // courtyard
    if (haveCourtyard) {
        auto &courtyard = footprint.courtyard;
        double2 courtyardSize = bodySize + courtyard.margin * 2.0;
        courtyardSize.x *= mirror;

        writeRectangle(s, position + courtyard.offset, courtyardSize, 0.05, "F.CrtYd");
    }

    // fabrication layer
    if (haveFabrication) {
        auto &fabrication = footprint.fabrication;
        double2 fabricationSize = bodySize + fabrication.margin * 2.0;
        fabricationSize.x *= mirror;

        writeFabRectangle(s, position + fabrication.offset, fabricationSize, footprint.orientation);
    }

    // lines
    for (auto &line : footprint.lines) {
        writeLine(s, footprint.position, line);
    }

    // rectangles
    for (auto &rectangle : footprint.rectangles) {
        writeRectangle(s, footprint.position, rectangle);
    }

    // circles
    for (auto &circle : footprint.circles) {
        writeCircle(s, footprint.position, circle);
    }

    // pads (also clip away the silkscreen)
    for (auto &pad : footprint.pads) {
        switch (pad.type) {
        case Footprint::Pad::Type::SINGLE:
            writeSingle(s, footprint, pad, clips);
            break;
        case Footprint::Pad::Type::DUAL:
            writeDual(s, footprint, pad, clips);
            break;
        case Footprint::Pad::Type::QUAD:
            writeQuad(s, footprint.position, pad, clips);
            break;
        case Footprint::Pad::Type::GRID:
            writeGrid(s, footprint.position, pad, clips);
            break;
        }
    }

    // silkscreen
    if (haveSilkscreen) {
        // subtract pads from silkscreen
        clipper.AddClip(clips);
        clipper2::Paths64 closedPahts;
        clipper2::Paths64 openPaths;
        clipper.Execute(clipper2::ClipType::Difference, clipper2::FillRule::NonZero, closedPahts, openPaths);
        writeSilkscreenPaths(s, closedPahts);
        writeSilkscreenPaths(s, openPaths, 1);
    }

    s << "  (embedded_fonts no)" << std::endl;

    // 3D model
    if (haveBody) {
        //s << "  (model \"" << name << ".wrl\" (offset (xyz 0 0 0)) (scale (xyz 1 1 1)) (rotate (xyz 0 0 0)))" << std::endl;
        s << "  (model \"" << name << ".step\" (offset (xyz 0 0 0)) (scale (xyz 1 1 1)) (rotate (xyz 0 0 0)))" << std::endl;
    }

    // footer
    s << ")" << std::endl;

    s.close();

    // return true when vrml should be generated
    return haveBody;
}

int main(int argc, const char **argv) {
    //Footprint footprint;
    //footprint.body.size = {1, 1, 1};
    //generateStep(".", "foo.step", footprint);
    //return 0;

    if (argc < 2)
        return 1;
    fs::path path = argv[1];
    //fs::path path = "footprints.json";
    std::cout << "Read " << path << std::endl;

    // read footprints
    std::map<std::string, Footprint> footprints;
    readJson(path, footprints);

    // generate footprints
    for (const auto &[name, footprint] : footprints) {
        // check if footprint is a template
        if (footprint.template_)
            continue;
        std::cout << name << std::endl;
        auto dir = path.parent_path();
        if (generateFootprint(dir, name, footprint)) {
            //generateVrml(dir, name, footprint);
            generateStep(dir, name, footprint);
        }
    }

    return 0;
}
