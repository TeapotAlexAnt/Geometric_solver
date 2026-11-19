#include "HelperFunctions.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

namespace cad {

int HelperFunctions::queryPointAt(const GeomRepository& repo, double x, double y, double radius,
                                  const std::vector<int>& excluded_ids) {
    int best_id = -1;
    double best_dist = radius;
    const std::unordered_set<int> excluded(excluded_ids.begin(), excluded_ids.end());
    for (const auto& p : repo.points) {
        double d = std::hypot(p->x - x, p->y - y);
        if (d <= best_dist && excluded.count(p->id) == 0) {
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

std::vector<int> HelperFunctions::queryBox(const GeomRepository& repo, double x1, double y1, double x2,
                                           double y2) {
    const double left = std::min(x1, x2), right = std::max(x1, x2);
    const double bottom = std::min(y1, y2), top = std::max(y1, y2);
    const auto inside = [&](const PointFigure& p) {
        return left <= p.x && p.x <= right && bottom <= p.y && p.y <= top;
    };
    // Clip the segment's parameter interval against both coordinate ranges.
    const auto intersects = [&](const PointFigure& a, const PointFigure& b) {
        double enter = 0.0, leave = 1.0;
        const auto clip = [&](double start, double end, double lo, double hi) {
            if (start == end) return lo <= start && start <= hi;
            const double t1 = (lo - start) / (end - start);
            const double t2 = (hi - start) / (end - start);
            enter = std::max(enter, std::min(t1, t2));
            leave = std::min(leave, std::max(t1, t2));
            return enter <= leave;
        };
        return clip(a.x, b.x, left, right) && clip(a.y, b.y, bottom, top);
    };

    std::vector<int> ids;
    for (const auto& p : repo.points) {
        if (!p->segment_id && inside(*p)) ids.push_back(p->id);
    }
    for (const auto& s : repo.segments) {
        const auto& a = *s->p1_ref;
        const auto& b = *s->p2_ref;
        if (x1 > x2 ? intersects(a, b) : inside(a) && inside(b)) ids.push_back(s->id);
    }
    return ids;
}

int HelperFunctions::countEquations(const GeomRepository& repo) {
    int total_eqs = 0;
    for (const auto& r : repo.restrictions) {
        if (r.type != RestrictionType::COINCIDENT) total_eqs += r.equationCount();
    }
    return total_eqs;
}

std::vector<std::vector<const PointFigure*>>
HelperFunctions::groupCoincidentPoints(const GeomRepository& repo,
                                       const std::vector<Restriction>& extra_restrictions) {
    const size_t count = repo.points.size();
    std::vector<size_t> parents(count);
    std::vector<size_t> sizes(count, 1);
    std::iota(parents.begin(), parents.end(), 0);
    std::unordered_map<int, size_t> indices;
    for (size_t i = 0; i < count; ++i)
        indices.emplace(repo.points[i]->id, i);

    const auto findRoot = [&](size_t i) {
        while (parents[i] != i) {
            parents[i] = parents[parents[i]];
            i = parents[i];
        }
        return i;
    };
    const auto merge = [&](const std::vector<Restriction>& restrictions) {
        for (const auto& r : restrictions) {
            if (r.type != RestrictionType::COINCIDENT) continue;
            size_t a = findRoot(indices.at(r.p_ids[0]));
            size_t b = findRoot(indices.at(r.p_ids[1]));
            if (a == b) continue;
            if (sizes[a] < sizes[b]) std::swap(a, b);
            parents[b] = a;
            sizes[a] += sizes[b];
        }
    };
    merge(repo.restrictions);
    merge(extra_restrictions);

    std::vector<size_t> group_indices(count, count);
    std::vector<std::vector<const PointFigure*>> groups;
    for (size_t i = 0; i < count; ++i) {
        size_t root = findRoot(i);
        if (group_indices[root] == count) {
            group_indices[root] = groups.size();
            groups.emplace_back();
        }
        groups[group_indices[root]].push_back(repo.points[i].get());
    }
    return groups;
}

int HelperFunctions::computeDOF(const GeomRepository& repo) {
    return static_cast<int>(groupCoincidentPoints(repo).size()) * 2 - countEquations(repo);
}

} // namespace cad
