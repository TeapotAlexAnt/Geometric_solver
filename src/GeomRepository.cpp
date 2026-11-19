#include "GeomRepository.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

namespace cad {

void GeomRepository::clear() {
    points.clear();
    segments.clear();
    restrictions.clear();
    next_id = 1;
}

std::shared_ptr<PointFigure> GeomRepository::addPoint(double x, double y, const std::string& label) {
    if (next_id == std::numeric_limits<int>::max()) return nullptr;
    int id = next_id++;
    auto pt = std::make_shared<PointFigure>(id, x, y, label);
    points.push_back(pt);
    return pt;
}

std::shared_ptr<SegmentFigure> GeomRepository::addSegment(int p1_id, int p2_id, const std::string& label) {
    if (p1_id <= 0 || p2_id <= 0) return nullptr;
    return addSegment(0, 0, 0, 0, label, p1_id, p2_id);
}

std::shared_ptr<SegmentFigure> GeomRepository::addSegment(double x1, double y1, double x2, double y2,
                                                          const std::string& label, int snap_start,
                                                          int snap_end) {
    const auto* first = findPoint(snap_start);
    const auto* last = findPoint(snap_end);
    if ((snap_start && !first) || (snap_end && !last) || (first && first == last)) return nullptr;
    if (first) {
        x1 = first->x;
        y1 = first->y;
    }
    if (last) {
        x2 = last->x;
        y2 = last->y;
    }
    if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2) || !std::isfinite(y2) ||
        next_id > std::numeric_limits<int>::max() - 3 - (first != nullptr) - (last != nullptr)) {
        return nullptr;
    }
    int id = next_id++;
    const auto name = label.empty() ? "L" + std::to_string(id) : label;
    auto start = std::make_shared<PointFigure>(next_id++, x1, y1, name + ".1", id);
    auto end = std::make_shared<PointFigure>(next_id++, x2, y2, name + ".2", id);
    auto seg = std::make_shared<SegmentFigure>(id, start->id, end->id, start.get(), end.get(), name);
    points.push_back(start);
    points.push_back(end);
    segments.push_back(seg);
    if (first) addRestriction({0, RestrictionType::COINCIDENT, {start->id, first->id}, {}, 0, 0});
    if (last) addRestriction({0, RestrictionType::COINCIDENT, {end->id, last->id}, {}, 0, 0});
    return seg;
}

int GeomRepository::addRestriction(const Restriction& r) {
    Restriction copy = r;
    if (copy.id <= 0) {
        if (next_id == std::numeric_limits<int>::max()) return -1;
        copy.id = next_id++;
    }
    restrictions.push_back(copy);
    return copy.id;
}

bool GeomRepository::removeRestriction(int rid) {
    auto it = std::remove_if(restrictions.begin(), restrictions.end(),
                             [rid](const Restriction& r) { return r.id == rid; });
    if (it == restrictions.end()) return false;
    restrictions.erase(it, restrictions.end());
    return true;
}

bool GeomRepository::removePrimitive(int id) {
    const auto* point = findPoint(id);
    if (point && point->segment_id) return removePrimitive(point->segment_id);

    int start_id = id;
    int end_id = id;
    if (!point) {
        const auto segment =
            std::find_if(segments.begin(), segments.end(), [id](const auto& s) { return s->id == id; });
        if (segment == segments.end()) return false;
        start_id = (*segment)->p1_id;
        end_id = (*segment)->p2_id;
        segments.erase(segment);
    }
    const auto removesPoint = [=](int pid) { return pid == start_id || pid == end_id; };
    restrictions.erase(std::remove_if(restrictions.begin(), restrictions.end(),
                                      [&](const Restriction& r) {
                                          return std::any_of(r.p_ids.begin(), r.p_ids.end(), removesPoint) ||
                                                 std::find(r.seg_ids.begin(), r.seg_ids.end(), id) !=
                                                     r.seg_ids.end();
                                      }),
                       restrictions.end());
    points.erase(
        std::remove_if(points.begin(), points.end(), [&](const auto& p) { return removesPoint(p->id); }),
        points.end());
    return true;
}

PointFigure* GeomRepository::findPoint(int id) const {
    for (const auto& p : points) {
        if (p->id == id) return p.get();
    }
    return nullptr;
}

SegmentFigure* GeomRepository::findSegment(int id) const {
    for (const auto& s : segments) {
        if (s->id == id) return s.get();
    }
    return nullptr;
}

bool GeomRepository::isValidRestriction(const Restriction& r) const {
    constexpr int point_counts[] = {2, 2, 4, 4, 2, 2, 4, 3, 1};
    int code = static_cast<int>(r.type);
    if (code < 1 || code > 9 || r.p_ids.size() != point_counts[code - 1] || !std::isfinite(r.value) ||
        !std::isfinite(r.value2)) {
        return false;
    }
    return std::all_of(r.p_ids.begin(), r.p_ids.end(), [&](int id) { return findPoint(id); }) &&
           std::all_of(r.seg_ids.begin(), r.seg_ids.end(), [&](int id) { return findSegment(id); });
}

void GeomRepository::writeJsonFields(std::ostream& out) const {
    out << "\"primitives\":[";
    bool first = true;
    for (const auto& p : points) {
        if (!first) out << ",";
        first = false;
        out << p->toJson();
    }
    for (const auto& s : segments) {
        if (!first) out << ",";
        first = false;
        out << s->toJson();
    }
    out << "],\"restrictions\":[";
    for (size_t i = 0; i < restrictions.size(); ++i) {
        if (i) out << ",";
        out << restrictions[i].toJson();
    }
    out << ']';
}

std::string GeomRepository::serializeToJson() const {
    std::ostringstream ss;
    ss << '{';
    writeJsonFields(ss);
    ss << '}';
    return ss.str();
}

} // namespace cad
