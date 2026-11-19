#include "GeomSolver.hpp"
#include "HelperFunctions.hpp"
#include <iterator>
#include <unordered_map>
#include <unordered_set>

namespace cad {

OptimizeResult GeomSolver::solve(GeomRepository& repo, const std::vector<Restriction>& extra_restrictions,
                                 const std::vector<int>& dragged_point_ids) {
    const auto groups = HelperFunctions::groupCoincidentPoints(repo, extra_restrictions);
    const std::unordered_set<int> dragged(dragged_point_ids.begin(), dragged_point_ids.end());
    std::unordered_map<int, int> pointIdToVarIdx;
    std::vector<double> initial_params;
    std::vector<double> variable_weights;
    std::vector<std::vector<int>> point_groups;

    for (const auto& group : groups) {
        const int x_index = static_cast<int>(initial_params.size());
        auto& ids = point_groups.emplace_back();
        double x = 0.0, y = 0.0;
        double drag_x = 0.0, drag_y = 0.0;
        int count = 0, drag_count = 0;
        for (const auto* point : group) {
            pointIdToVarIdx.emplace(point->id, x_index);
            ids.push_back(point->id);
            ++count;
            x += point->x / count - x / count;
            y += point->y / count - y / count;
            if (dragged.count(point->id)) {
                ++drag_count;
                drag_x += point->x / drag_count - drag_x / drag_count;
                drag_y += point->y / drag_count - drag_y / drag_count;
            }
        }
        // При перетаскивании всей группы следует за курсором её перемещаемая точка.
        initial_params.insert(initial_params.end(), {drag_count ? drag_x : x, drag_count ? drag_y : y});
        const double weight = count + 24.0 * drag_count;
        variable_weights.insert(variable_weights.end(), {weight, weight});
    }

    std::vector<const Restriction*> equations;
    int total_eqs = 0;
    const auto collect = [&](const std::vector<Restriction>& restrictions) {
        for (const auto& restriction : restrictions) {
            if (restriction.type == RestrictionType::COINCIDENT) continue;
            equations.push_back(&restriction);
            total_eqs += restriction.equationCount();
        }
    };
    collect(repo.restrictions);
    collect(extra_restrictions);

    auto evaluate = [&](const std::vector<double>& params, std::vector<double>& residuals,
                        SparseMatrix& gradients) {
        residuals.reserve(total_eqs);
        gradients.reserve(total_eqs);
        std::vector<double> local_residuals;
        SparseMatrix local_gradients;
        std::vector<std::map<int, SparseRow>> hessians;
        for (const auto* restriction : equations) {
            restriction->evaluateAnalytical(params, pointIdToVarIdx, local_residuals, local_gradients,
                                            hessians);
            residuals.insert(residuals.end(), local_residuals.begin(), local_residuals.end());
            gradients.insert(gradients.end(), std::make_move_iterator(local_gradients.begin()),
                             std::make_move_iterator(local_gradients.end()));
        }
    };

    auto result = CoreAlgo::optimize(evaluate, initial_params, total_eqs, 60, 1e-6, variable_weights);
    result.point_groups = std::move(point_groups);
    // Даже без уравнений Лагранжа совпадение должно обновить координаты всех точек группы.
    for (auto& point : repo.points) {
        const int i = pointIdToVarIdx.at(point->id);
        point->x = result.final_params[i];
        point->y = result.final_params[i + 1];
    }
    return result;
}

} // namespace cad
