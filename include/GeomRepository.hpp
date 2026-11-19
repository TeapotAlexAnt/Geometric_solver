#pragma once
#include "Figure.hpp"
#include "Restrictions.hpp"
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

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

    void clear();

    std::shared_ptr<PointFigure> addPoint(double x, double y, const std::string& label = "");

    std::shared_ptr<SegmentFigure> addSegment(int p1_id, int p2_id, const std::string& label = "");

    std::shared_ptr<SegmentFigure> addSegment(double x1, double y1, double x2, double y2,
                                              const std::string& label = "", int snap_start = 0,
                                              int snap_end = 0);

    int addRestriction(const Restriction& r);

    bool removeRestriction(int rid);

    bool removePrimitive(int id);

    PointFigure* findPoint(int id) const;

    SegmentFigure* findSegment(int id) const;

    bool isValidRestriction(const Restriction& r) const;

    // Serializer JSON (соответствует фиолетовой стрелке serializer -> JSON на схеме бригады)
    void writeJsonFields(std::ostream& out) const;
    std::string serializeToJson() const;
};

} // namespace cad
