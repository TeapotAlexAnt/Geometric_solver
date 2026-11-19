#include "UseCases.hpp"
#include "GeomSolver.hpp"
#include "HelperFunctions.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

namespace cad {

namespace {

template <typename T> void writeArray(std::ostream& out, const std::vector<T>& values) {
    out << '[';
    for (size_t i = 0; i < values.size(); ++i) {
        if (i) out << ',';
        out << values[i];
    }
    out << ']';
}

template <typename T> void writeArray(std::ostream& out, const std::vector<std::vector<T>>& rows) {
    out << '[';
    for (size_t i = 0; i < rows.size(); ++i) {
        if (i) out << ',';
        writeArray(out, rows[i]);
    }
    out << ']';
}

void writeArray(std::ostream& out, const SparseMatrix& rows) {
    out << '[';
    for (size_t i = 0; i < rows.size(); ++i) {
        if (i) out << ',';
        out << '[';
        bool first = true;
        for (const auto& [column, value] : rows[i]) {
            if (!first) out << ',';
            first = false;
            out << '[' << column << ',' << value << ']';
        }
        out << ']';
    }
    out << ']';
}

} // namespace

GeomRepository& ApiHandler::getRepository() { return repo; }

std::string ApiHandler::getAllPrimitives() const {
    const auto total_vars = static_cast<int>(last_solve_result.final_params.size());
    const auto total_eqs = static_cast<int>(last_solve_result.final_residuals.size());
    const int dof = total_vars - total_eqs;
    std::ostringstream ss;
    ss << std::setprecision(std::numeric_limits<double>::max_digits10);
    ss << '{';
    repo.writeJsonFields(ss);
    ss << ",\"solver\":{"
       << "\"converged\":" << (last_solve_result.converged ? "true" : "false")
       << ",\"iterations\":" << last_solve_result.iterations
       << ",\"max_delta\":" << last_solve_result.max_delta
       << ",\"max_residual\":" << last_solve_result.max_residual << ",\"dof\":" << dof
       << ",\"point_groups\":";
    writeArray(ss, last_solve_result.point_groups);
    ss << ",\"last_jacobian\":";
    writeArray(ss, last_solve_result.last_jacobian);
    ss << ",\"last_rhs\":";
    writeArray(ss, last_solve_result.last_rhs);
    ss << ",\"last_solution\":";
    writeArray(ss, last_solve_result.last_solution);
    ss << ",\"final_residuals\":";
    writeArray(ss, last_solve_result.final_residuals);
    ss << ",\"final_gradients\":";
    writeArray(ss, last_solve_result.final_gradients);
    ss << "},\"dofInfo\":{\"totalVars\":" << total_vars << ",\"totalEqs\":" << total_eqs << ",\"dof\":" << dof
       << "}}";
    return ss.str();
}

int ApiHandler::createPoint(double x, double y, const std::string& label) {
    if (!std::isfinite(x) || !std::isfinite(y)) return -1;
    auto pt = repo.addPoint(x, y, label);
    if (!pt) return -1;
    solveSketch();
    return pt->id;
}

int ApiHandler::createSegment(int p1_id, int p2_id, const std::string& label) {
    auto seg = repo.addSegment(p1_id, p2_id, label);
    if (seg) solveSketch();
    return seg ? seg->id : -1;
}

int ApiHandler::createSegmentAt(double x1, double y1, double x2, double y2, int snap_start, int snap_end,
                                const std::string& label) {
    const auto segment = repo.addSegment(x1, y1, x2, y2, label, snap_start, snap_end);
    if (segment) solveSketch();
    return segment ? segment->id : -1;
}

bool ApiHandler::deletePrimitive(int id) {
    bool ok = repo.removePrimitive(id);
    if (ok) last_solve_result = GeomSolver::solve(repo);
    return ok;
}

int ApiHandler::applyRestriction(int type_code, const std::vector<int>& p_ids,
                                 const std::vector<int>& seg_ids, double val1, double val2) {
    Restriction r;
    r.id = 0;
    r.type = static_cast<RestrictionType>(type_code);
    r.p_ids = p_ids;
    r.seg_ids = seg_ids;
    r.value = val1;
    r.value2 = val2;
    if (!repo.isValidRestriction(r)) return -1;
    int rid = repo.addRestriction(r);
    if (rid > 0) solveSketch();
    return rid;
}

bool ApiHandler::removeRestriction(int rid) {
    bool ok = repo.removeRestriction(rid);
    if (ok) last_solve_result = GeomSolver::solve(repo);
    return ok;
}

bool ApiHandler::updateRestriction(int id, double value, double value2) {
    if (!std::isfinite(value) || !std::isfinite(value2)) return false;
    auto it = std::find_if(repo.restrictions.begin(), repo.restrictions.end(),
                           [id](const Restriction& r) { return r.id == id; });
    if (it == repo.restrictions.end()) return false;
    it->value = value;
    it->value2 = value2;
    solveSketch();
    return true;
}

void ApiHandler::solveSketch() { last_solve_result = GeomSolver::solve(repo); }

void ApiHandler::clearSketch() {
    repo.clear();
    solveSketch();
}

void ApiHandler::replaceSketch(GeomRepository replacement) {
    OptimizeResult result = GeomSolver::solve(replacement);
    repo = std::move(replacement);
    last_solve_result = std::move(result);
}

int ApiHandler::queryPoint(double x, double y, double radius, const std::vector<int>& excluded_ids) const {
    return HelperFunctions::queryPointAt(repo, x, y, radius, excluded_ids);
}

int ApiHandler::querySegment(double x, double y, double radius) const {
    return HelperFunctions::querySegmentAt(repo, x, y, radius);
}

void ApiHandler::movePoint(int point_id, double target_x, double target_y) {
    PointFigure* pt = repo.findPoint(point_id);
    if (!pt || !std::isfinite(target_x) || !std::isfinite(target_y)) return;
    pt->x = target_x;
    pt->y = target_y;
    // При перетаскивании точки задаем её начальное приближение в точке курсора,
    // а принцип наименьших изменений Лагранжиана (слайд 27) автоматически удерживает её
    // максимально близко к курсору при соблюдении всех наложенных ограничений!
    last_solve_result = GeomSolver::solve(repo, {}, {point_id});
}

void ApiHandler::moveSegment(int segment_id, double dx, double dy) {
    SegmentFigure* seg = repo.findSegment(segment_id);
    if (!seg || !std::isfinite(dx) || !std::isfinite(dy)) return;
    seg->p1_ref->x += dx;
    seg->p1_ref->y += dy;
    seg->p2_ref->x += dx;
    seg->p2_ref->y += dy;
    last_solve_result = GeomSolver::solve(repo, {}, {seg->p1_id, seg->p2_id});
}

const OptimizeResult& ApiHandler::getLastSolveResult() const { return last_solve_result; }

} // namespace cad
