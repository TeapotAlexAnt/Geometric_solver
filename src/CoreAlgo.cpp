#include "CoreAlgo.hpp"
#include <cmath>
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

OptimizeResult CoreAlgo::optimize(EvalCallback eval_f_df, const std::vector<double>& initial_params,
                                  int num_equations, int max_iterations, double eps,
                                  const std::vector<double>& variable_weights) {
    OptimizeResult res;
    res.final_params = initial_params;

    int n = static_cast<int>(initial_params.size());
    int m = num_equations;
    if (n == 0 || m == 0) {
        return res;
    }

    std::vector<double> p = initial_params;
    std::vector<double> lambda(m, 0.0);
    int total_dim =
        m + n; // первые m уравнений: dF/dlambda_j = 0, следующие n уравнений: dF/ddp_i = 0 (как на слайде 31)

    for (int iter = 0; iter < max_iterations; ++iter) {
        res.iterations = iter + 1;

        std::vector<double> f_vec;
        SparseMatrix J_f;
        eval_f_df(p, f_vec, J_f);

        double max_f = 0.0;
        for (int j = 0; j < m; ++j) {
            if (std::fabs(f_vec[j]) > max_f) max_f = std::fabs(f_vec[j]);
        }
        res.max_residual = max_f;

        if (max_f < eps && iter > 0) {
            res.converged = true;
            break;
        }

        // Формируем матрицу Якоби A размера (m + n) x (m + n) и правую часть b для системы (**) со слайда 31:
        // Неизвестные СЛАУ: z = [dp_1, ..., dp_n, lambda_1, ..., lambda_m]^T
        // Уравнения 0..m-1 (dF/dlambda_j = 0):
        //   sum_i (df_j / d(dp_i)) * dp_i + 0 * lambda = -f_j(p)
        // Уравнения m..m+n-1 (dF/d(dp_i) = 0):
        //   w_i * dp_i + sum_j (df_j / d(dp_i)) * lambda_j = 0
        SparseMatrix A(total_dim);
        std::vector<double> b(total_dim, 0.0);

        // Верхний блок (m строк): уравнения ограничений dF/dlambda_j = f_j(p + dp) = 0
        for (int j = 0; j < m; ++j) {
            for (const auto& [i, value] : J_f[j]) {
                A[j].emplace(i, value);
                A[m + i].emplace(n + j, value);
            }
            // Слабая регуляризация диагонали множителей Лагранжа для устойчивости при зависимых ограничениях
            A[j][n + j] = -1e-9;
            b[j] = -f_vec[j];
        }

        // Нижний блок (n строк): уравнения стационарности Лагранжиана dF/ddp_i = dp_i + sum_j lambda_j *
        // df_j/ddp_i = 0
        for (int i = 0; i < n; ++i) {
            A[m + i][i] = variable_weights.empty() ? 1.0 : variable_weights[i];
        }

        if (iter == 0) {
            res.last_jacobian = A;
            res.last_rhs = b;
        }

        // Решаем СЛАУ собственным методом Гаусса CoreAlgo::solve(A, b)
        std::vector<double> sol = CoreAlgo::solve(std::move(A), std::move(b));
        if (iter == 0) res.last_solution = sol;

        // Ограничение максимального шага (damping) для глобальной сходимости сильно нелинейных ограничений
        double max_dp = 0.0;
        for (int i = 0; i < n; ++i) {
            if (std::fabs(sol[i]) > max_dp) max_dp = std::fabs(sol[i]);
        }

        double step_scale = 1.0;
        const double max_allowed_step = 80.0;
        if (max_dp > max_allowed_step) {
            step_scale = max_allowed_step / max_dp;
        }

        // Применяем приращения к варьируемым параметрам p (слайд 32)
        double norm_inf = 0.0;
        for (int i = 0; i < n; ++i) {
            double dp_i = sol[i] * step_scale;
            p[i] += dp_i;
            if (std::fabs(dp_i) > norm_inf) norm_inf = std::fabs(dp_i);
        }
        for (int j = 0; j < m; ++j) {
            lambda[j] = sol[n + j];
        }

        res.max_delta = norm_inf;
        res.lagrange_multipliers = lambda;

        // Проверка условия окончания итерационного процесса ||dp||_inf < eps
        if (norm_inf < eps && max_f < eps * 10.0) {
            res.converged = true;
            break;
        }
    }

    // Финальная проверка невязки
    std::vector<double> f_final;
    SparseMatrix J_final;
    eval_f_df(p, f_final, J_final);
    double max_f_final = 0.0;
    for (double val : f_final) {
        if (std::fabs(val) > max_f_final) max_f_final = std::fabs(val);
    }
    res.max_residual = max_f_final;
    res.converged = (max_f_final < 1e-3);
    res.final_params = p;
    res.final_residuals = std::move(f_final);
    res.final_gradients = std::move(J_final);
    return res;
}

} // namespace cad
