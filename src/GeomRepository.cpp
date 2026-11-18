#include "GeomRepository.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace cad {

void GeomRepository::clear() {
    points.clear();
    segments.clear();
    restrictions.clear();
    next_id = 1;
}

std::shared_ptr<PointFigure> GeomRepository::addPoint(double x, double y, const std::string& label) {
    int id = next_id++;
    auto pt = std::make_shared<PointFigure>(id, x, y, label);
    points.push_back(pt);
    return pt;
}

std::shared_ptr<SegmentFigure> GeomRepository::addSegment(int p1_id, int p2_id, const std::string& label) {
    PointFigure* r1 = findPoint(p1_id);
    PointFigure* r2 = findPoint(p2_id);
    if (!r1 || !r2 || p1_id == p2_id) return nullptr;
    for (const auto& segment : segments) {
        if ((segment->p1_id == p1_id && segment->p2_id == p2_id) ||
            (segment->p1_id == p2_id && segment->p2_id == p1_id)) {
            return segment;
        }
    }
    int id = next_id++;
    auto seg = std::make_shared<SegmentFigure>(id, p1_id, p2_id, r1, r2, label);
    segments.push_back(seg);
    return seg;
}

int GeomRepository::addRestriction(const Restriction& r) {
    Restriction copy = r;
    if (copy.id <= 0) {
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
    // Если удаляется отрезок
    auto sit = std::remove_if(segments.begin(), segments.end(),
                              [id](const std::shared_ptr<SegmentFigure>& s) { return s->id == id; });
    if (sit != segments.end()) {
        segments.erase(sit, segments.end());
        // Удаляем ограничения, ссылающиеся на этот отрезок
        restrictions.erase(std::remove_if(restrictions.begin(), restrictions.end(),
                                          [id](const Restriction& r) {
                                              return std::find(r.seg_ids.begin(), r.seg_ids.end(), id) !=
                                                     r.seg_ids.end();
                                          }),
                           restrictions.end());
        return true;
    }

    // Если удаляется точка — сначала удаляем инцидентные отрезки и ограничения
    auto pit = std::find_if(points.begin(), points.end(),
                            [id](const std::shared_ptr<PointFigure>& p) { return p->id == id; });
    if (pit != points.end()) {
        std::vector<int> segs_to_remove;
        for (const auto& s : segments) {
            if (s->p1_id == id || s->p2_id == id) {
                segs_to_remove.push_back(s->id);
            }
        }
        for (int sid : segs_to_remove) {
            removePrimitive(sid);
        }
        restrictions.erase(std::remove_if(restrictions.begin(), restrictions.end(),
                                          [id](const Restriction& r) {
                                              return std::find(r.p_ids.begin(), r.p_ids.end(), id) !=
                                                     r.p_ids.end();
                                          }),
                           restrictions.end());
        points.erase(pit);
        return true;
    }
    return false;
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

void GeomRepository::relinkSegments() {
    for (auto& s : segments) {
        s->p1_ref = findPoint(s->p1_id);
        s->p2_ref = findPoint(s->p2_id);
    }
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
