#pragma once

//#include "clipper2.hpp"
#include "double3.hpp"
#include <optional>
#include <string>
#include <vector>


// shapes
constexpr double CIRCLE = 0.5; // oval if width not equal to height
constexpr double ROUNDRECT = 0.25; // 25% (KiCad default)
constexpr double ROUNDRECT10 = 0.1; // 10%
constexpr double ROUNDRECT5 = 0.05; // 5%
constexpr double RECTANGLE = 0;


struct Footprint {
    // footprint type
    enum class Type {
        // detect type, SMD if at least one SMD pad is present
        DETECT,

        THROUGH_HOLE,

        SMD,
    };

    // body
    struct Body {
        // body size
        double3 size;

        // body offset (z-direction applies to generated 3d model)
        double3 offset;
    };

    // silkscreen
    struct Silkscreen {
        bool enabled = true;

        // pin 1 marker
        enum class Marker {
            // no pin 1 marker
            NONE,

            // dot as pin 1 marker
            DOT,

            // bar on side with first and last pin (assuming circular numbering)
            BAR
        };

        // pin 1 marker
        Marker marker = Marker::DOT;

        // margin around body
        double2 margin;

        // silkscreen offset
        double2 offset;
    };

    // courtyard
    struct Courtyard {
        bool enabled = true;

        // margin around body
        double2 margin;

        // courtyard offset
        double2 offset;
    };

    // fabrication layer
    struct Fabrication {
        bool enabled = true;

        // margin around body
        double2 margin;

        // fabrication layer offset
        double2 offset;
    };


    // pin orientation
    enum class Orientation {
        // pin 1 marker is at botton-left position
        BOTTOM_LEFT,

        // pin 1 marker is at bottom-right position
        BOTTOM_RIGHT,

        // pin 1 marker is at top-left position
        TOP_LEFT,

        // pin 1 marker is at top-right position
        TOP_RIGHT,
    };

    // pad or pad array
    struct Pad {
        enum class Type {
            // single line of pads
            SINGLE,

            // two lines of pads
            DUAL,

            //
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

        // package type for generating multiple pads
        Type type = Type::SINGLE;

        // global position of pad or center of multiple pads
        double2 position;

        // distance between pad rows
        double2 distance;

        // pitch between pads
        double pitch = 0;

        // shift of rows along the pad line (results in zigzag pattern for type dual)
        double shift = 0;

        // size of pad
        double2 size;

        // offset of pad (copper) relative to position
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

        // layers
        bool back = false;
        bool mask = true;
        bool paste = true;

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
        double width = 0.1;
        std::vector<double2> points;
    };

    struct Rectangle {
        std::string layer;
        double width = 0.1;
        double2 p1;
        double2 p2;
    };

    struct Circle {
        std::string layer;
        double width = 0.1;
        bool fill = false;
        double2 center = {0, 0};
        double radius = 0.5;
    };


    // true if this is a template, i.e. no footprint gets generated
    bool template_ = false;

    // description of footprint
    std::string description;

    // through-hole or smd
    Type type = Type::DETECT;

    // global position, applies to everything
    double2 position;

    // body (body size is used for silkscreen)
    // migrate .json files: search: "body":\s*\[(.+?)\] replace: "body": {"size": [$1]}
    Body body;

    // silkscreen
    Silkscreen silkscreen;

    // courtyard
    Courtyard courtyard;

    // fabrication layer
    Fabrication fabrication;

    // orientation of footprint (position of pin 1 marker)
    Orientation orientation = Orientation::BOTTOM_LEFT;

    // properties
    bool excludeFromBom = false;
    bool excludeFromPosFiles = false;
    bool allowSolderMaskBridges = false;

    // list of pads (pad arrays)
    std::vector<Pad> pads;

    // shapes
    std::vector<Line> lines;
    std::vector<Rectangle> rectangles;
    std::vector<Circle> circles;


    // get type of footrpint
    Type getType() const {
        if (this->type == Footprint::Type::DETECT) {
            // detect footprint type
            for (auto &pad : this->pads) {
                //if (pad.size.positive() && pad.drillSize.positive())
                //    return Type::THROUGH_HOLE;
                if (pad.size.positive() && !pad.drillSize.positive())
                    return Type::SMD;
            }
            return Type::THROUGH_HOLE;
        }
        return this->type;
    }
};
