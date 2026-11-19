#pragma once
#include "CoreAlgo.hpp"
#include "GeomRepository.hpp"
#include <string>

namespace cad {

struct PointMoveTarget;

/**
 * Render Usecase, Tool_1 Usecase, Pointer Usecase и API Handler
 * (соответствуют одноименным блокам на схеме бригады).
 */
class ApiHandler {
  private:
    GeomRepository repo;
    OptimizeResult last_solve_result;
    bool auto_solve_enabled = true;
    bool solve_result_current = true;

    void refreshSketch();
    void solveWithTargets(const std::vector<PointMoveTarget>& targets);

  public:
    GeomRepository& getRepository();

    // Render Usecase -> getAllPrimitives
    std::string getAllPrimitives() const;

    // Tool_1 Usecase: создание и удаление точек/отрезков и наложение/снятие 9 типов ограничений
    int createPoint(double x, double y, const std::string& label = "");

    int createSegment(int p1_id, int p2_id, const std::string& label = "");

    int createSegmentAt(double x1, double y1, double x2, double y2, int snap_start = 0, int snap_end = 0,
                        const std::string& label = "");

    bool deletePrimitive(int id);
    bool deletePrimitives(const std::vector<int>& ids);

    int applyRestriction(int type_code, const std::vector<int>& p_ids, const std::vector<int>& seg_ids,
                         double val1, double val2 = 0.0);

    bool removeRestriction(int rid);

    bool updateRestriction(int id, double value, double value2);

    void solveSketch();

    void setAutoSolve(bool enabled);

    void clearSketch();

    void replaceSketch(GeomRepository replacement);

    int queryPoint(double x, double y, double radius, const std::vector<int>& excluded_ids = {}) const;

    int querySegment(double x, double y, double radius) const;

    std::vector<int> queryBox(double x1, double y1, double x2, double y2) const;

    // Pointer Usecase: перемещение точки с пересчетом геометрии эскиза в реальном времени
    void movePoint(int point_id, double target_x, double target_y);

    void moveSegment(int segment_id, double dx, double dy);

    const OptimizeResult& getLastSolveResult() const;
};

} // namespace cad
