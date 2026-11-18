#pragma once
#include "GeomRepository.hpp"
#include "CoreAlgo.hpp"
#include <unordered_map>

namespace cad {

/**
 * Geom Solver (соответствует блоку Geom Solver на схеме бригады):
 * solve(geom, restrictions, new_restrictions) -> new_geom
 */
class GeomSolver {
public:
    static OptimizeResult solve(
        GeomRepository& repo,
        const std::vector<Restriction>& extra_restrictions = {}
    ) {
        int nPoints = static_cast<int>(repo.points.size());
        int nVars = nPoints * 2;
        std::vector<double> initial_params(nVars, 0.0);
        std::unordered_map<int, int> pointIdToVarIdx;

        for (int i = 0; i < nPoints; ++i) {
            pointIdToVarIdx[repo.points[i]->id] = i * 2;
            initial_params[i * 2]     = repo.points[i]->x;
            initial_params[i * 2 + 1] = repo.points[i]->y;
        }

        // Объединяем основные ограничения репозитория и дополнительные (например, при перетаскивании курсором)
        std::vector<Restriction> all_restrictions = repo.restrictions;
        for (const auto& er : extra_restrictions) {
            all_restrictions.push_back(er);
        }

        int total_eqs = 0;
        for (const auto& r : all_restrictions) {
            total_eqs += r.equationCount();
        }

        if (nVars == 0 || total_eqs == 0) {
            OptimizeResult empty_res;
            empty_res.converged = true;
            empty_res.iterations = 0;
            empty_res.max_delta = 0.0;
            empty_res.max_residual = 0.0;
            empty_res.final_params = initial_params;
            return empty_res;
        }

        auto eval_callback = [&](
            const std::vector<double>& X,
            std::vector<double>& f_vec,
            std::vector<std::vector<double>>& J_f,
            std::vector<std::vector<std::vector<double>>>& H_f
        ) {
            f_vec.assign(total_eqs, 0.0);
            J_f.assign(total_eqs, std::vector<double>(nVars, 0.0));
            H_f.assign(total_eqs, std::vector<std::vector<double>>(nVars, std::vector<double>(nVars, 0.0)));

            int row = 0;
            for (const auto& r : all_restrictions) {
                std::vector<double> f_loc;
                std::vector<std::vector<double>> df_loc;
                std::vector<std::vector<std::vector<double>>> d2f_loc;
                r.evaluateAnalytical(X, pointIdToVarIdx, f_loc, df_loc, d2f_loc);

                for (size_t k = 0; k < f_loc.size(); ++k) {
                    f_vec[row] = f_loc[k];
                    J_f[row]   = df_loc[k];
                    H_f[row]   = d2f_loc[k];
                    ++row;
                }
            }
        };

        OptimizeResult res = CoreAlgo::optimize(eval_callback, initial_params, total_eqs, 60, 1e-6);

        // Обновляем координаты точек в GeomRepository (new_geom)
        for (int i = 0; i < nPoints; ++i) {
            repo.points[i]->x = res.final_params[i * 2];
            repo.points[i]->y = res.final_params[i * 2 + 1];
        }
        repo.relinkSegments();
        return res;
    }
};

} // namespace cad
