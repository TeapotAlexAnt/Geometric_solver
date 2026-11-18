#include "CoreAlgo.hpp"
#include <cmath>
#include <utility>

namespace cad {

std::vector<double> CoreAlgo::solve(std::vector<std::vector<double>> A, std::vector<double> b) {
    int n = static_cast<int>(b.size());
    if (n == 0) return {};

    // Прямой ход метода Гаусса с частичным выбором главного элемента (partial pivoting)
    for (int k = 0; k < n; ++k) {
        int max_row = k;
        double max_val = std::fabs(A[k][k]);
        for (int i = k + 1; i < n; ++i) {
            double v = std::fabs(A[i][k]);
            if (v > max_val) {
                max_val = v;
                max_row = i;
            }
        }

        // Перестановка строк, если найден больший ведущий элемент
        if (max_row != k) {
            std::vector<double> tmp_row = A[k];
            A[k] = A[max_row];
            A[max_row] = tmp_row;

            double tmp_b = b[k];
            b[k] = b[max_row];
            b[max_row] = tmp_b;
        }

        // Регуляризация Тихонова на случай вырожденной / переопределенной системы ограничений
        if (std::fabs(A[k][k]) < 1e-11) {
            A[k][k] = (A[k][k] >= 0.0 ? 1e-11 : -1e-11);
        }

        double pivot = A[k][k];
        for (int i = k + 1; i < n; ++i) {
            double factor = A[i][k] / pivot;
            if (std::fabs(factor) < 1e-16) continue;
            A[i][k] = 0.0;
            for (int j = k + 1; j < n; ++j) {
                A[i][j] -= factor * A[k][j];
            }
            b[i] -= factor * b[k];
        }
    }

    // Обратный ход метода Гаусса
    std::vector<double> x(n, 0.0);
    for (int i = n - 1; i >= 0; --i) {
        double sum = b[i];
        for (int j = i + 1; j < n; ++j) {
            sum -= A[i][j] * x[j];
        }
        double diag = A[i][i];
        if (std::fabs(diag) < 1e-12) {
            x[i] = 0.0;
        } else {
            x[i] = sum / diag;
        }
    }
    return x;
}

OptimizeResult CoreAlgo::optimize(EvalCallback eval_f_df, const std::vector<double>& initial_params,
                                  int num_equations, int max_iterations, double eps,
                                  const std::vector<double>& variable_weights) {
    OptimizeResult res;
    res.final_params = initial_params;
    res.converged = true;
    res.iterations = 0;
    res.max_delta = 0.0;
    res.max_residual = 0.0;

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
        std::vector<std::vector<double>> J_f;
        std::vector<std::vector<std::vector<double>>> H_f;
        eval_f_df(p, f_vec, J_f, H_f);

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
        //   (1 + mu) * dp_i + sum_j (df_j / d(dp_i)) * lambda_j = 0
        std::vector<std::vector<double>> A(total_dim, std::vector<double>(total_dim, 0.0));
        std::vector<double> b(total_dim, 0.0);

        // Верхний блок (m строк): уравнения ограничений dF/dlambda_j = f_j(p + dp) = 0
        for (int j = 0; j < m; ++j) {
            for (int i = 0; i < n; ++i) {
                A[j][i] = J_f[j][i];
            }
            // Слабая регуляризация диагонали множителей Лагранжа для устойчивости при зависимых ограничениях
            A[j][n + j] = -1e-9;
            b[j] = -f_vec[j];
        }

        // Нижний блок (n строк): уравнения стационарности Лагранжиана dF/ddp_i = dp_i + sum_j lambda_j *
        // df_j/ddp_i = 0
        for (int i = 0; i < n; ++i) {
            A[m + i][i] = variable_weights.empty() ? 1.0 : variable_weights[i];
            // Перетаскивание увеличивает вес координат, следующих за курсором.
            for (int j = 0; j < m; ++j) {
                A[m + i][n + j] = J_f[j][i];
            }
            b[m + i] = 0.0;
        }

        if (iter == 0) {
            res.last_jacobian = A;
            res.last_rhs = b;
        }

        // Решаем СЛАУ собственным методом Гаусса CoreAlgo::solve(A, b)
        std::vector<double> sol = CoreAlgo::solve(A, b);
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
    std::vector<std::vector<double>> J_final;
    std::vector<std::vector<std::vector<double>>> H_final;
    eval_f_df(p, f_final, J_final, H_final);
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
