#pragma once
#include "SparseMatrix.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace cad {

/**
 * 9 типов геометрических ограничений (слайды 28–29 презентации РК6):
 * 1. COINCIDENT      - Совпадение двух точек (общие координаты в СЛАУ)
 * 2. DISTANCE        - Расстояние между двумя точками / длина отрезка (1 уравнение)
 * 3. PARALLEL        - Параллельность двух отрезков (1 уравнение)
 * 4. PERPENDICULAR   - Перпендикулярность двух отрезков (1 уравнение)
 * 5. VERTICAL        - Вертикальность отрезка (1 уравнение)
 * 6. HORIZONTAL      - Горизонтальность отрезка (1 уравнение)
 * 7. ANGLE           - Угол между двумя отрезками (1 уравнение)
 * 8. POINT_ON_LINE   - Принадлежность точки прямой (1 уравнение)
 * 9. FIX_POINT       - Фиксация точки (2 уравнения)
 */
enum class RestrictionType {
    COINCIDENT = 1,
    DISTANCE = 2,
    PARALLEL = 3,
    PERPENDICULAR = 4,
    VERTICAL = 5,
    HORIZONTAL = 6,
    ANGLE = 7,
    POINT_ON_LINE = 8,
    FIX_POINT = 9
};

struct Restriction {
    int id;
    RestrictionType type;
    // Индексы точек в глобальном списке (для точек и концов отрезков):
    // Для COINCIDENT, DISTANCE: p_ids = {p1_id, p2_id}
    // Для PARALLEL, PERPENDICULAR, ANGLE: p_ids = {p1_id, p2_id, p3_id, p4_id}, seg_ids = {seg1_id, seg2_id}
    // Для VERTICAL, HORIZONTAL: p_ids = {p1_id, p2_id}, seg_ids = {seg_id}
    // Для POINT_ON_LINE: p_ids = {point_id, line_p1_id, line_p2_id}, seg_ids = {seg_id}
    // Для FIX_POINT: p_ids = {point_id}
    std::vector<int> p_ids;
    std::vector<int> seg_ids;
    double value;  // расстояние d, угол alpha (в градусах) или x_c
    double value2; // y_c для FIX_POINT

    int equationCount() const;

    std::string typeName() const;

    /**
     * Вычисление аналитических значений функций ограничений f_j(p + dp)
     * и аналитических частных производных первого (df_j / d(dp_i)) и второго (d^2 f_j / d(dp_i)d(dp_k))
     * порядков. Все аналитические формулы жестко зашиты в код согласно требованию ТЗ и слайдам 28-31.
     */
    void evaluateAnalytical(
        const std::vector<double>& X,                        // текущие координаты X_i = p_i + dp_i
        const std::unordered_map<int, int>& pointIdToVarIdx, // point_id -> индекс x (y имеет индекс +1)
        std::vector<double>& f_out,                          // значения уравнений
        SparseMatrix& df_out,                                // градиенты
        std::vector<std::map<int, SparseRow>>& d2f_out       // разреженные гессианы
    ) const;

    std::string toJson() const;
};

} // namespace cad
