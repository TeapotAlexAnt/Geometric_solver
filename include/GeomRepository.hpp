#pragma once
#include "Figure.hpp"
#include "Restrictions.hpp"
#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

namespace cad {

/**
 * Geom Repository (соответствует блоку Geom Repository на схеме бригады):
 * - Хранит Geom (точки и отрезки) и Restrictions (геометрические ограничения)
 * - Содержит JSON-сериализатор (serializer JSON)
 */
class GeomRepository {
public:
    std::vector<std::shared_ptr<PointFigure>> points;
    std::vector<std::shared_ptr<SegmentFigure>> segments;
    std::vector<Restriction> restrictions;
    int next_id = 1;

    void clear() {
        points.clear();
        segments.clear();
        restrictions.clear();
        next_id = 1;
    }

    std::shared_ptr<PointFigure> addPoint(double x, double y, const std::string& label = "") {
        int id = next_id++;
        auto pt = std::make_shared<PointFigure>(id, x, y, label);
        points.push_back(pt);
        return pt;
    }

    std::shared_ptr<SegmentFigure> addSegment(int p1_id, int p2_id, const std::string& label = "") {
        PointFigure* r1 = findPoint(p1_id);
        PointFigure* r2 = findPoint(p2_id);
        if (!r1 || !r2 || p1_id == p2_id) return nullptr;
        int id = next_id++;
        auto seg = std::make_shared<SegmentFigure>(id, p1_id, p2_id, r1, r2, label);
        segments.push_back(seg);
        return seg;
    }

    int addRestriction(const Restriction& r) {
        Restriction copy = r;
        if (copy.id <= 0) {
            copy.id = next_id++;
        }
        restrictions.push_back(copy);
        return copy.id;
    }

    bool removeRestriction(int rid) {
        auto it = std::remove_if(restrictions.begin(), restrictions.end(),
                                 [rid](const Restriction& r) { return r.id == rid; });
        if (it == restrictions.end()) return false;
        restrictions.erase(it, restrictions.end());
        return true;
    }

    bool removePrimitive(int id) {
        // Если удаляется отрезок
        auto sit = std::remove_if(segments.begin(), segments.end(),
                                  [id](const std::shared_ptr<SegmentFigure>& s) { return s->id == id; });
        if (sit != segments.end()) {
            segments.erase(sit, segments.end());
            // Удаляем ограничения, ссылающиеся на этот отрезок
            restrictions.erase(
                std::remove_if(restrictions.begin(), restrictions.end(),
                               [id](const Restriction& r) {
                                   return std::find(r.seg_ids.begin(), r.seg_ids.end(), id) != r.seg_ids.end();
                               }),
                restrictions.end()
            );
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
            restrictions.erase(
                std::remove_if(restrictions.begin(), restrictions.end(),
                               [id](const Restriction& r) {
                                   return std::find(r.p_ids.begin(), r.p_ids.end(), id) != r.p_ids.end();
                               }),
                restrictions.end()
            );
            points.erase(pit);
            return true;
        }
        return false;
    }

    PointFigure* findPoint(int id) const {
        for (const auto& p : points) {
            if (p->id == id) return p.get();
        }
        return nullptr;
    }

    SegmentFigure* findSegment(int id) const {
        for (const auto& s : segments) {
            if (s->id == id) return s.get();
        }
        return nullptr;
    }

    void relinkSegments() {
        for (auto& s : segments) {
            s->p1_ref = findPoint(s->p1_id);
            s->p2_ref = findPoint(s->p2_id);
        }
    }

    // Serializer JSON (соответствует фиолетовой стрелке serializer -> JSON на схеме бригады)
    std::string serializeToJson() const {
        std::ostringstream ss;
        ss << "{\"primitives\":[";
        bool first = true;
        for (const auto& p : points) {
            if (!first) ss << ",";
            first = false;
            ss << p->toJson();
        }
        for (const auto& s : segments) {
            if (!first) ss << ",";
            first = false;
            ss << s->toJson();
        }
        ss << "],\"restrictions\":[";
        for (size_t i = 0; i < restrictions.size(); ++i) {
            if (i) ss << ",";
            ss << restrictions[i].toJson();
        }
        ss << "]}";
        return ss.str();
    }
};

} // namespace cad
