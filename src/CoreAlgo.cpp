#include "CoreAlgo.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace cad {

std::vector<double> CoreAlgo::solve(SparseMatrix A, std::vector<double> b) {
    int n = static_cast<int>(b.size());
    if (n == 0) return {};

    // Прямой ход метода Гаусса с частичным выбором главного элемента (partial pivoting)
    for (int k = 0; k < n; ++k) {
        int max_row = k;
        double max_val = 0.0;
        for (int i = k; i < n; ++i) {
            const auto entry = A[i].find(k);
            if (entry != A[i].end() && std::fabs(entry->second) > max_val) {
                max_val = std::fabs(entry->second);
                max_row = i;
            }
        }

        // Перестановка строк, если найден больший ведущий элемент
        if (max_row != k) {
            std::swap(A[k], A[max_row]);
            std::swap(b[k], b[max_row]);
        }

        // Регуляризация Тихонова на случай вырожденной / переопределенной системы ограничений
        double& pivot = A[k][k];
        if (std::fabs(pivot) < 1e-11) pivot = std::copysign(1e-11, pivot);

        for (int i = k + 1; i < n; ++i) {
            auto entry = A[i].find(k);
            if (entry == A[i].end()) continue;
            double factor = entry->second / pivot;
            A[i].erase(entry);
            if (std::fabs(factor) < 1e-16) continue;
            for (auto it = A[k].upper_bound(k); it != A[k].end(); ++it) {
                const auto [column, value] = *it;
                auto& coefficient = A[i][column];
                coefficient -= factor * value;
                if (coefficient == 0.0) A[i].erase(column);
            }
            b[i] -= factor * b[k];
        }
    }

    // Обратный ход метода Гаусса
    std::vector<double> x(n, 0.0);
    for (int i = n - 1; i >= 0; --i) {
        double sum = b[i];
        for (auto it = A[i].upper_bound(i); it != A[i].end(); ++it) {
            sum -= it->second * x[it->first];
        }
        x[i] = sum / A[i].at(i);
    }
    return x;
}

namespace {

double maxAbs(const std::vector<double>& values) {
    double result = 0.0;
    for (double value : values) {
        if (!std::isfinite(value)) return std::numeric_limits<double>::infinity();
        result = std::max(result, std::fabs(value));
    }
    return result;
}

} // namespace

OptimizeResult CoreAlgo::optimize(EvalCallback eval_f_df, const std::vector<double>& initial_params,
                                  int num_equations, int max_iterations, double eps,
                                  const std::vector<double>& variable_weights,
                                  const std::vector<double>& reference_params) {
    const auto& reference = reference_params.empty() ? initial_params : reference_params;
    const int n = static_cast<int>(initial_params.size());
    const int m = num_equations;
    const int total_dim = m + n;
    OptimizeResult res;
    if (n == 0 || m == 0) {
        res.final_params = reference;
        return res;
    }

    const auto weight = [&](int i) { return variable_weights.empty() ? 1.0 : variable_weights[i]; };
    const auto cost = [&](const std::vector<double>& params) {
        double result = 0.0;
        for (int i = 0; i < n; ++i) {
            const double displacement = params[i] - reference[i];
            result += 0.5 * weight(i) * displacement * displacement;
        }
        return result;
    };

    constexpr double constraint_tolerance = 1e-3;
    std::vector<double> p = initial_params;
    std::vector<double> f;
    SparseMatrix jacobian;
    // Если цель уже допустима, значение штрафа в ней равно нулю.
    if (reference != initial_params) {
        eval_f_df(reference, f, jacobian);
        if (maxAbs(f) < constraint_tolerance) p = reference;
    }
    if (p != reference || f.empty()) {
        f.clear();
        jacobian.clear();
        eval_f_df(p, f, jacobian);
    }

    res.final_params = initial_params;
    const double initial_cost =
        maxAbs(f) < constraint_tolerance ? cost(p) : std::numeric_limits<double>::infinity();
    double best_cost = std::numeric_limits<double>::infinity();
    std::vector<double> penalty(m, 1.0);
    for (int iter = 0;; ++iter) {
        const double current_cost = cost(p);
        const double max_residual = maxAbs(f);
        if (max_residual < constraint_tolerance && current_cost < best_cost) {
            best_cost = current_cost;
            res.final_params = p;
        }
        if (iter >= max_iterations || !std::isfinite(current_cost) || !std::isfinite(max_residual)) break;
        res.iterations = iter + 1;

        // Ищем поправку d к текущим координатам p. Штраф за итоговое смещение
        // вычисляется в p + d относительно одной и той же опорной геометрии.
        SparseMatrix A(total_dim);
        std::vector<double> b(total_dim, 0.0);
        for (int j = 0; j < m; ++j) {
            for (const auto& [i, value] : jacobian[j]) {
                A[j].emplace(i, value);
                A[m + i].emplace(n + j, value);
            }
            // Регуляризация множителей для зависимых уравнений ограничений.
            A[j][n + j] = -1e-9;
            b[j] = -f[j];
        }
        for (int i = 0; i < n; ++i) {
            A[m + i][i] = weight(i);
            b[m + i] = -weight(i) * (p[i] - reference[i]);
        }

        if (iter == 0) {
            res.last_jacobian = A;
            res.last_rhs = b;
        }
        const auto step = solve(std::move(A), b);
        if (!std::isfinite(maxAbs(step))) break;
        if (iter == 0) res.last_solution = step;

        double max_delta = 0.0;
        double cost_slope = 0.0;
        for (int i = 0; i < n; ++i) {
            max_delta = std::max(max_delta, std::fabs(step[i]));
            cost_slope -= b[m + i] * step[i];
        }
        if (max_delta < eps) break;

        // Штраф за невязку используется только для выбора длины шага.
        // У каждого уравнения свой вес, поскольку невязки измеряются в разных единицах.
        double merit_slope = cost_slope;
        for (int j = 0; j < m; ++j) {
            penalty[j] = std::max(penalty[j], 1.0 + 2.0 * std::fabs(step[n + j]));
            double linearized_residual = f[j];
            for (const auto& [i, value] : jacobian[j]) {
                linearized_residual += value * step[i];
            }
            merit_slope += penalty[j] * (std::fabs(linearized_residual) - std::fabs(f[j]));
        }
        if (!(merit_slope < 0.0) || !std::isfinite(merit_slope)) break;

        std::vector<double> trial(n);
        std::vector<double> trial_f;
        SparseMatrix trial_jacobian;
        constexpr double max_step = 80.0;
        double scale = std::min(1.0, max_step / max_delta);
        constexpr int max_backtracks = 20;
        bool accepted = false;
        for (int backtrack = 0; backtrack < max_backtracks && scale * max_delta >= eps;
             ++backtrack, scale *= 0.5) {
            for (int i = 0; i < n; ++i) {
                trial[i] = p[i] + scale * step[i];
            }
            if (!std::isfinite(maxAbs(trial))) continue;
            trial_f.clear();
            trial_jacobian.clear();
            eval_f_df(trial, trial_f, trial_jacobian);
            double merit_change = cost(trial) - current_cost;
            for (int j = 0; j < m; ++j) {
                merit_change += penalty[j] * (std::fabs(trial_f[j]) - std::fabs(f[j]));
            }
            accepted = std::isfinite(merit_change) && merit_change <= 1e-4 * scale * merit_slope;
            if (accepted) break;
        }
        if (!accepted) break;

        p.swap(trial);
        f.swap(trial_f);
        jacobian.swap(trial_jacobian);
        res.max_delta = scale * max_delta;
        res.lagrange_multipliers.assign(step.begin() + n, step.end());
    }

    // При остановке внутри допустимой области используем последний результат уточнения.
    if (maxAbs(f) < constraint_tolerance && cost(p) <= initial_cost) res.final_params = std::move(p);
    eval_f_df(res.final_params, res.final_residuals, res.final_gradients);
    res.max_residual = maxAbs(res.final_residuals);
    res.converged = res.max_residual < constraint_tolerance;
    return res;
}

} // namespace cad
