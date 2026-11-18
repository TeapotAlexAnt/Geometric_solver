#pragma once
#include "GeomRepository.hpp"
#include <cmath>
#include <vector>

namespace cad {

/**
 * Helper Functions -> query (соответствует блоку Helper Functions / query на схеме бригады):
 * Поиск объектов по координатам курсора, проверка связности и подсчет степеней свободы (DOF).
 */
class HelperFunctions {
public:
    static int queryPointAt(const GeomRepository& repo, double x, double y, double radius = 10.0) {
        int best_id = -1;
        double best_dist = radius;
        for (const auto& p : repo.points) {
            double d = std::hypot(p->x - x, p->y - y);
            if (d <= best_dist) {
                best_dist = d;
                best_id = p->id;
            }
        }
        return best_id;
    }

    static int querySegmentAt(const GeomRepository& repo, double x, double y, double tolerance = 8.0) {
        int best_id = -1;
        double best_dist = tolerance;
        for (const auto& s : repo.segments) {
            if (!s->p1_ref || !s->p2_ref) continue;
            double x1 = s->p1_ref->x, y1 = s->p1_ref->y;
            double x2 = s->p2_ref->x, y2 = s->p2_ref->y;
            double dx = x2 - x1, dy = y2 - y1;
            double len2 = dx * dx + dy * dy;
            double t = (len2 > 1e-12) ? ((x - x1) * dx + (y - y1) * dy) / len2 : 0.0;
            if (t < 0.0) t = 0.0;
            if (t > 1.0) t = 1.0;
            double proj_x = x1 + t * dx;
            double proj_y = y1 + t * dy;
            double d = std::hypot(x - proj_x, y - proj_y);
            if (d <= best_dist) {
                best_dist = d;
                best_id = s->id;
            }
        }
        return best_id;
    }

    static int computeDOF(const GeomRepository& repo) {
        int total_vars = static_cast<int>(repo.points.size()) * 2;
        int total_eqs = 0;
        for (const auto& r : repo.restrictions) {
            total_eqs += r.equationCount();
        }
        return total_vars - total_eqs;
    }
};

} // namespace cad
