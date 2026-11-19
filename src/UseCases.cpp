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

void writeValue(std::ostream& out, int value) { out << value; }

void writeValue(std::ostream& out, double value) {
    if (std::isfinite(value))
        out << value;
    else
        out << "null";
}

template <typename T> void writeArray(std::ostream& out, const std::vector<T>& values) {
    out << '[';
    for (size_t i = 0; i < values.size(); ++i) {
        if (i) out << ',';
        writeValue(out, values[i]);
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
            out << '[' << column << ',';
            writeValue(out, value);
            out << ']';
        }
        out << ']';
    }
    out << ']';
}

} // namespace

GeomRepository& ApiHandler::getRepository() { return repo; }

std::string ApiHandler::getAllPrimitives() const {
    const int total_vars = solve_result_current
                               ? static_cast<int>(last_solve_result.final_params.size())
                               : static_cast<int>(HelperFunctions::groupCoincidentPoints(repo).size()) * 2;
    const int total_eqs = solve_result_current ? static_cast<int>(last_solve_result.final_residuals.size())
                                               : HelperFunctions::countEquations(repo);
    const int dof = total_vars - total_eqs;
    std::ostringstream ss;
    ss << std::setprecision(std::numeric_limits<double>::max_digits10);
    ss << '{';
    repo.writeJsonFields(ss);
    ss << ",\"solver\":{"
       << "\"auto_solve\":" << (auto_solve_enabled ? "true" : "false")
       << ",\"current\":" << (solve_result_current ? "true" : "false")
       << ",\"converged\":" << (solve_result_current && last_solve_result.converged ? "true" : "false")
       << ",\"iterations\":" << last_solve_result.iterations
       << ",\"max_delta\":" << last_solve_result.max_delta << ",\"max_residual\":";
    writeValue(ss, last_solve_result.max_residual);
    ss << ",\"dof\":" << dof << ",\"point_groups\":";
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
    refreshSketch();
    return pt->id;
}

int ApiHandler::createSegment(int p1_id, int p2_id, const std::string& label) {
    auto seg = repo.addSegment(p1_id, p2_id, label);
    if (seg) refreshSketch();
    return seg ? seg->id : -1;
}

int ApiHandler::createSegmentAt(double x1, double y1, double x2, double y2, int snap_start, int snap_end,
                                const std::string& label) {
    const auto segment = repo.addSegment(x1, y1, x2, y2, label, snap_start, snap_end);
    if (segment) refreshSketch();
    return segment ? segment->id : -1;
}

bool ApiHandler::deletePrimitive(int id) { return deletePrimitives({id}); }

bool ApiHandler::deletePrimitives(const std::vector<int>& ids) {
    bool changed = false;
    for (const int id : ids) {
        if (repo.removePrimitive(id)) changed = true;
    }
    if (changed) refreshSketch();
    return changed;
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
    if (rid > 0) refreshSketch();
    return rid;
}

bool ApiHandler::removeRestriction(int rid) {
    bool ok = repo.removeRestriction(rid);
    if (ok) refreshSketch();
    return ok;
}

bool ApiHandler::updateRestriction(int id, double value, double value2) {
    if (!std::isfinite(value) || !std::isfinite(value2)) return false;
    auto it = std::find_if(repo.restrictions.begin(), repo.restrictions.end(),
                           [id](const Restriction& r) { return r.id == id; });
    if (it == repo.restrictions.end()) return false;
    it->value = value;
    it->value2 = value2;
    refreshSketch();
    return true;
}

void ApiHandler::solveSketch() { solveWithTargets({}); }

void ApiHandler::setAutoSolve(bool enabled) {
    if (auto_solve_enabled == enabled) return;
    auto_solve_enabled = enabled;
    if (enabled) solveSketch();
}

void ApiHandler::refreshSketch() {
    if (auto_solve_enabled) {
        solveSketch();
    } else if (solve_result_current) {
        last_solve_result = {};
        solve_result_current = false;
    }
}

void ApiHandler::solveWithTargets(const std::vector<PointMoveTarget>& targets) {
    last_solve_result = GeomSolver::solve(repo, {}, targets);
    solve_result_current = true;
}

void ApiHandler::clearSketch() {
    repo.clear();
    refreshSketch();
}

void ApiHandler::replaceSketch(GeomRepository replacement) {
    if (auto_solve_enabled) {
        last_solve_result = GeomSolver::solve(replacement);
        solve_result_current = true;
    }
    repo = std::move(replacement);
    if (!auto_solve_enabled) refreshSketch();
}

int ApiHandler::queryPoint(double x, double y, double radius, const std::vector<int>& excluded_ids) const {
    return HelperFunctions::queryPointAt(repo, x, y, radius, excluded_ids);
}

int ApiHandler::querySegment(double x, double y, double radius) const {
    return HelperFunctions::querySegmentAt(repo, x, y, radius);
}

std::vector<int> ApiHandler::queryBox(double x1, double y1, double x2, double y2) const {
    return HelperFunctions::queryBox(repo, x1, y1, x2, y2);
}

void ApiHandler::movePoint(int point_id, double target_x, double target_y) {
    auto* point = repo.findPoint(point_id);
    if (!point || !std::isfinite(target_x) || !std::isfinite(target_y)) return;
    if (auto_solve_enabled) {
        solveWithTargets({{point_id, target_x, target_y}});
    } else {
        point->x = target_x;
        point->y = target_y;
        refreshSketch();
    }
}

void ApiHandler::moveSegment(int segment_id, double dx, double dy) {
    const SegmentFigure* seg = repo.findSegment(segment_id);
    if (!seg || !std::isfinite(dx) || !std::isfinite(dy)) return;
    const double x1 = seg->p1_ref->x + dx, y1 = seg->p1_ref->y + dy;
    const double x2 = seg->p2_ref->x + dx, y2 = seg->p2_ref->y + dy;
    if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2) || !std::isfinite(y2)) return;
    if (auto_solve_enabled) {
        solveWithTargets({{seg->p1_id, x1, y1}, {seg->p2_id, x2, y2}});
    } else {
        seg->p1_ref->x = x1;
        seg->p1_ref->y = y1;
        seg->p2_ref->x = x2;
        seg->p2_ref->y = y2;
        refreshSketch();
    }
}

const OptimizeResult& ApiHandler::getLastSolveResult() const { return last_solve_result; }

} // namespace cad
