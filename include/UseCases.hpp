#pragma once
#include "GeomRepository.hpp"
#include "HelperFunctions.hpp"
#include "GeomSolver.hpp"
#include <string>
#include <sstream>

namespace cad {

/**
 * Render Usecase, Tool_1 Usecase, Pointer Usecase и API Handler
 * (соответствуют одноименным блокам на схеме бригады).
 */
class ApiHandler {
private:
    GeomRepository repo;
    OptimizeResult last_solve_result;

public:
    ApiHandler() {
        last_solve_result.converged = true;
        last_solve_result.iterations = 0;
        last_solve_result.max_delta = 0.0;
        last_solve_result.max_residual = 0.0;
    }

    GeomRepository& getRepository() { return repo; }

    // Render Usecase -> getAllPrimitives
    std::string getAllPrimitives() const {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6);
        ss << "{\"primitives\":[";
        bool first = true;
        for (const auto& p : repo.points) {
            if (!first) ss << ",";
            first = false;
            ss << p->toJson();
        }
        for (const auto& s : repo.segments) {
            if (!first) ss << ",";
            first = false;
            ss << s->toJson();
        }
        ss << "],\"restrictions\":[";
        for (size_t i = 0; i < repo.restrictions.size(); ++i) {
            if (i) ss << ",";
            ss << repo.restrictions[i].toJson();
        }
        ss << "],\"solver\":{"
           << "\"converged\":" << (last_solve_result.converged ? "true" : "false")
           << ",\"iterations\":" << last_solve_result.iterations
           << ",\"max_delta\":" << last_solve_result.max_delta
           << ",\"max_residual\":" << last_solve_result.max_residual
           << ",\"dof\":" << HelperFunctions::computeDOF(repo)
           << "}}";
        return ss.str();
    }

    // Tool_1 Usecase: создание и удаление точек/отрезков и наложение/снятие 9 типов ограничений
    int createPoint(double x, double y) {
        auto pt = repo.addPoint(x, y);
        return pt->id;
    }

    int createSegment(int p1_id, int p2_id) {
        auto seg = repo.addSegment(p1_id, p2_id);
        return seg ? seg->id : -1;
    }

    bool deletePrimitive(int id) {
        bool ok = repo.removePrimitive(id);
        if (ok) last_solve_result = GeomSolver::solve(repo);
        return ok;
    }

    int applyRestriction(int type_code, const std::vector<int>& p_ids, const std::vector<int>& seg_ids, double val1, double val2 = 0.0) {
        Restriction r;
        r.id = 0;
        r.type = static_cast<RestrictionType>(type_code);
        r.p_ids = p_ids;
        r.seg_ids = seg_ids;
        r.value = val1;
        r.value2 = val2;
        int rid = repo.addRestriction(r);
        last_solve_result = GeomSolver::solve(repo);
        return rid;
    }

    bool removeRestriction(int rid) {
        bool ok = repo.removeRestriction(rid);
        if (ok) last_solve_result = GeomSolver::solve(repo);
        return ok;
    }

    // Pointer Usecase: перемещение точки с пересчетом геометрии эскиза в реальном времени
    void movePoint(int point_id, double target_x, double target_y) {
        PointFigure* pt = repo.findPoint(point_id);
        if (!pt) return;
        pt->x = target_x;
        pt->y = target_y;
        // При перетаскивании точки задаем её начальное приближение в точке курсора,
        // а принцип наименьших изменений Лагранжиана (слайд 27) автоматически удерживает её
        // максимально близко к курсору при соблюдении всех наложенных ограничений!
        last_solve_result = GeomSolver::solve(repo);
    }

    const OptimizeResult& getLastSolveResult() const {
        return last_solve_result;
    }
};

} // namespace cad
