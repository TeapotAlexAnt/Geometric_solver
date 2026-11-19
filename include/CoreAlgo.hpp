#pragma once
#include "SparseMatrix.hpp"
#include <functional>
#include <vector>

namespace cad {

struct OptimizeResult {
    bool converged = true;
    int iterations = 0;
    double max_delta = 0.0;
    double max_residual = 0.0;
    std::vector<double> final_params;
    std::vector<double> lagrange_multipliers;
    SparseMatrix last_jacobian;
    std::vector<double> last_rhs;
    std::vector<double> last_solution;
    std::vector<double> final_residuals;
    SparseMatrix final_gradients;
    std::vector<std::vector<int>> point_groups;
};

/**
 * Core Algo (соответствует блоку Core Algo на архитектурной схеме бригады):
 * - solve(A, b): собственная реализация решения СЛАУ методом Гаусса с выбором главного элемента
 *   и регуляризацией без использования сторонних библиотек.
 * - optimize(f, df, params): решение задачи условной минимизации методом неопределенных
 *   множителей Лагранжа и методом Ньютона (слайды 27, 30-32 презентации РК6).
 */
class CoreAlgo {
  public:
    /**
     * Решение разреженной СЛАУ A * x = b методом Гаусса с выбором главного элемента по столбцу.
     */
    static std::vector<double> solve(SparseMatrix A, std::vector<double> b);

    /**
     * Функция optimize(f, df, params) по схеме бригады и слайдам 27, 30-32:
     * Целевая функция Лагранжа (принцип наименьших изменений):
     *   F(dp_1..dp_n, lambda_1..lambda_m) = 1/2 * sum(w_i * dp_i^2) + sum(lambda_j * f_j(p + dp)) -> min
     * Система уравнений (**):
     *   dF/d(lambda_j) = f_j(p + dp) = 0                       (j = 1..m)
     *   dF/d(dp_i)     = w_i * dp_i + sum_j lambda_j * df_j/d(dp_i) = 0  (i = 1..n)
     * На каждой итерации формируется матрица Якоби для (**), решается СЛАУ через CoreAlgo::solve(A, b),
     * приращения применяются к параметрам p, проверяется условие ||dp||_inf < eps и ||f||_inf < eps.
     */
    using EvalCallback =
        std::function<void(const std::vector<double>& X, std::vector<double>& f_vec, SparseMatrix& J_f)>;

    static OptimizeResult optimize(EvalCallback eval_f_df, const std::vector<double>& initial_params,
                                   int num_equations, int max_iterations = 50, double eps = 1e-6,
                                   const std::vector<double>& variable_weights = {});
};

} // namespace cad
