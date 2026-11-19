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
     * Минимизирует 1/2 * sum(w_i * (p_i - reference_i)^2) при f_j(p) = 0.
     * Опорные координаты reference фиксированы на весь расчёт и по умолчанию равны initial_params.
     * На каждой итерации ограничения линеаризуются в текущих координатах p, а поправка
     * выбирается методом множителей Лагранжа с контролем шага.
     */
    using EvalCallback =
        std::function<void(const std::vector<double>& X, std::vector<double>& f_vec, SparseMatrix& J_f)>;

    static OptimizeResult optimize(EvalCallback eval_f_df, const std::vector<double>& initial_params,
                                   int num_equations, int max_iterations = 50, double eps = 1e-6,
                                   const std::vector<double>& variable_weights = {},
                                   const std::vector<double>& reference_params = {});
};

} // namespace cad
