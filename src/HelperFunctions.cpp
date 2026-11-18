#include "HelperFunctions.hpp"
#include <cmath>

namespace cad {

int HelperFunctions::queryPointAt(const GeomRepository& repo, double x, double y, double radius) {
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

int HelperFunctions::querySegmentAt(const GeomRepository& repo, double x, double y, double tolerance) {
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

int HelperFunctions::countEquations(const GeomRepository& repo) {
    int total_eqs = 0;
    for (const auto& r : repo.restrictions) {
        total_eqs += r.equationCount();
    }
    return total_eqs;
}

int HelperFunctions::computeDOF(const GeomRepository& repo) {
    return static_cast<int>(repo.points.size()) * 2 - countEquations(repo);
}

} // namespace cad
