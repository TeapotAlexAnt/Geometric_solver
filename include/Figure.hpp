#pragma once
#include <string>
#include <vector>

namespace cad {

enum class FigureType { POINT, SEGMENT };

/**
 * Базовый класс геометрического примитива эскиза (Figure)
 * Соответствует блоку на схеме бригады:
 * Geom = Figure[]
 * class Figure: def params()
 */
class Figure {
  public:
    int id;
    FigureType type;
    std::string label;

    Figure(int id_, FigureType type_, const std::string& label_ = "") : id(id_), type(type_), label(label_) {}
    virtual ~Figure() = default;

    // Возвращает указатели на варьируемые параметры примитива в векторе состояния решателя
    virtual std::vector<double*> params() = 0;
    virtual std::string toJson() const = 0;
};

/**
 * Точка на плоскости P = (x, y)
 * Имеет 2 варьируемых параметра: [x, y]
 */
class PointFigure : public Figure {
  public:
    double x;
    double y;

    PointFigure(int id_, double x_, double y_, const std::string& label_ = "")
        : Figure(id_, FigureType::POINT, label_.empty() ? ("P" + std::to_string(id_)) : label_), x(x_),
          y(y_) {}

    std::vector<double*> params() override { return {&x, &y}; }

    std::string toJson() const override;
};

/**
 * Отрезок на плоскости L = (p1_id, p2_id)
 * Определяется двумя характерными точками P1=(x1,y1), P2=(x2,y2) (слайд 24 презентации РК6)
 */
class SegmentFigure : public Figure {
  public:
    int p1_id;
    int p2_id;
    PointFigure* p1_ref;
    PointFigure* p2_ref;

    SegmentFigure(int id_, int p1_, int p2_, PointFigure* r1 = nullptr, PointFigure* r2 = nullptr,
                  const std::string& label_ = "")
        : Figure(id_, FigureType::SEGMENT, label_.empty() ? ("L" + std::to_string(id_)) : label_), p1_id(p1_),
          p2_id(p2_), p1_ref(r1), p2_ref(r2) {}

    std::vector<double*> params() override {
        if (p1_ref && p2_ref) {
            return {&p1_ref->x, &p1_ref->y, &p2_ref->x, &p2_ref->y};
        }
        return {};
    }

    std::string toJson() const override;
};

} // namespace cad
