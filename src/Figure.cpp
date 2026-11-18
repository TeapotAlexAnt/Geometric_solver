#include "Figure.hpp"
#include <iomanip>
#include <limits>
#include <sstream>

namespace cad {

namespace {

void writeJsonString(std::ostream& out, const std::string& value) {
    constexpr char hex[] = "0123456789abcdef";
    out << '"';
    for (unsigned char c : value) {
        if (c < 0x20) {
            out << "\\u00" << hex[c >> 4] << hex[c & 15];
        } else {
            if (c == '"' || c == '\\') out << '\\';
            out << c;
        }
    }
    out << '"';
}

} // namespace

std::string PointFigure::toJson() const {
    std::ostringstream ss;
    ss << std::setprecision(std::numeric_limits<double>::max_digits10);
    ss << "{\"id\":" << id << ",\"type\":\"point\",\"label\":";
    writeJsonString(ss, label);
    ss << ",\"x\":" << x << ",\"y\":" << y << "}";
    return ss.str();
}

std::string SegmentFigure::toJson() const {
    std::ostringstream ss;
    ss << std::setprecision(std::numeric_limits<double>::max_digits10);
    ss << "{\"id\":" << id << ",\"type\":\"line\",\"label\":";
    writeJsonString(ss, label);
    ss << ",\"p1_id\":" << p1_id << ",\"p2_id\":" << p2_id;
    if (p1_ref && p2_ref) {
        ss << ",\"x1\":" << p1_ref->x << ",\"y1\":" << p1_ref->y << ",\"x2\":" << p2_ref->x
           << ",\"y2\":" << p2_ref->y;
    }
    ss << "}";
    return ss.str();
}

} // namespace cad
