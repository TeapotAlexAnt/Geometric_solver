#include "../include/UseCases.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace cad;

int main() {
    std::cout << "=== Тестирование вариационного геометрического решателя (C++ Core) ===" << std::endl;

    // Тест 1: Пример из презентации РК6 (слайды 30-31):
    // Фиксация точки P1 в (100, 100) и длина отрезка P1-P2 d = 50, при этом P2 изначально на расстоянии 40.
    {
        ApiHandler api;
        int p1 = api.createPoint(100.0, 100.0);
        int p2 = api.createPoint(140.0, 100.0); // исходная длина = 40 мм (слайд 16)
        int seg = api.createSegment(p1, p2);

        // Фиксация точки P1 (ограничение 9)
        api.applyRestriction(static_cast<int>(RestrictionType::FIX_POINT), {p1}, {}, 100.0, 100.0);
        // Расстояние между P1 и P2 = 50 мм (ограничение 2)
        api.applyRestriction(static_cast<int>(RestrictionType::DISTANCE), {p1, p2}, {seg}, 50.0);

        PointFigure* pt1 = api.getRepository().findPoint(p1);
        PointFigure* pt2 = api.getRepository().findPoint(p2);
        double dist = std::hypot(pt2->x - pt1->x, pt2->y - pt1->y);
        std::cout << "[Тест 1: Фиксация + Длина отрезка (слайд 30)] P1=(" << pt1->x << ", " << pt1->y
                  << "), P2=(" << pt2->x << ", " << pt2->y << "), L=" << dist
                  << ", итераций=" << api.getLastSolveResult().iterations << std::endl;
        assert(std::fabs(pt1->x - 100.0) < 1e-5);
        assert(std::fabs(pt1->y - 100.0) < 1e-5);
        assert(std::fabs(dist - 50.0) < 1e-5);
    }

    // Тест 2: Прямоугольник (горизонтальность, вертикальность, перпендикулярность, параллельность, длины)
    {
        ApiHandler api;
        int p1 = api.createPoint(0.0, 0.0);
        int p2 = api.createPoint(110.0, 15.0);
        int p3 = api.createPoint(105.0, 85.0);
        int p4 = api.createPoint(-10.0, 75.0);
        int s1 = api.createSegment(p1, p2);
        int s2 = api.createSegment(p2, p3);
        int s3 = api.createSegment(p3, p4);
        int s4 = api.createSegment(p4, p1);

        api.applyRestriction(static_cast<int>(RestrictionType::FIX_POINT), {p1}, {}, 0.0, 0.0);
        api.applyRestriction(static_cast<int>(RestrictionType::HORIZONTAL), {p1, p2}, {s1}, 0.0);
        api.applyRestriction(static_cast<int>(RestrictionType::VERTICAL), {p4, p1}, {s4}, 0.0);
        api.applyRestriction(static_cast<int>(RestrictionType::PARALLEL), {p1, p2, p4, p3}, {s1, s3}, 0.0);
        api.applyRestriction(static_cast<int>(RestrictionType::PERPENDICULAR), {p1, p2, p2, p3}, {s1, s2},
                             0.0);
        api.applyRestriction(static_cast<int>(RestrictionType::DISTANCE), {p1, p2}, {s1}, 120.0);
        api.applyRestriction(static_cast<int>(RestrictionType::DISTANCE), {p2, p3}, {s2}, 80.0);

        PointFigure* pt2 = api.getRepository().findPoint(p2);
        PointFigure* pt3 = api.getRepository().findPoint(p3);
        PointFigure* pt4 = api.getRepository().findPoint(p4);
        std::cout << "[Тест 2: Прямоугольник 120x80] P2=(" << pt2->x << ", " << pt2->y << "), P3=(" << pt3->x
                  << ", " << pt3->y << "), P4=(" << pt4->x << ", " << pt4->y << ")" << std::endl;
        assert(std::fabs(pt2->x - 120.0) < 1e-4 && std::fabs(pt2->y - 0.0) < 1e-4);
        assert(std::fabs(pt3->x - 120.0) < 1e-4 && std::fabs(pt3->y - 80.0) < 1e-4);
        assert(std::fabs(pt4->x - 0.0) < 1e-4 && std::fabs(pt4->y - 80.0) < 1e-4);
    }

    // Тест 3: Угол между отрезками (60 градусов), совпадение двух точек и принадлежность точки прямой
    {
        ApiHandler api;
        int p1 = api.createPoint(0.0, 0.0);
        int p2 = api.createPoint(100.0, 0.0);
        int p3 = api.createPoint(5.0, 5.0);
        int p4 = api.createPoint(80.0, 50.0);
        int p_mid = api.createPoint(40.0, 35.0);

        int s1 = api.createSegment(p1, p2);
        int s2 = api.createSegment(p3, p4);

        api.applyRestriction(static_cast<int>(RestrictionType::FIX_POINT), {p1}, {}, 0.0, 0.0);
        api.applyRestriction(static_cast<int>(RestrictionType::HORIZONTAL), {p1, p2}, {s1}, 0.0);
        api.applyRestriction(static_cast<int>(RestrictionType::DISTANCE), {p1, p2}, {s1}, 100.0);
        // Совпадение P1 и P3 (ограничение 1)
        api.applyRestriction(static_cast<int>(RestrictionType::COINCIDENT), {p1, p3}, {}, 0.0);
        // Длина второго отрезка = 100
        api.applyRestriction(static_cast<int>(RestrictionType::DISTANCE), {p3, p4}, {s2}, 100.0);
        // Угол между s1 и s2 = 60 градусов (ограничение 7)
        api.applyRestriction(static_cast<int>(RestrictionType::ANGLE), {p1, p2, p3, p4}, {s1, s2}, 60.0);
        // Принадлежность точки p_mid прямой отрезка s2 (ограничение 8)
        api.applyRestriction(static_cast<int>(RestrictionType::POINT_ON_LINE), {p_mid, p3, p4}, {s2}, 0.0);

        PointFigure* pt3 = api.getRepository().findPoint(p3);
        PointFigure* pt4 = api.getRepository().findPoint(p4);
        PointFigure* ptm = api.getRepository().findPoint(p_mid);
        double cross_line = (ptm->x - pt3->x) * (pt4->y - ptm->y) - (pt4->x - ptm->x) * (ptm->y - pt3->y);
        std::cout << "[Тест 3: Совпадение + Угол 60° + Точка на прямой] P3=(" << pt3->x << ", " << pt3->y
                  << "), P4=(" << pt4->x << ", " << pt4->y << "), P_mid=(" << ptm->x << ", " << ptm->y
                  << "), cross=" << cross_line << std::endl;
        assert(std::hypot(pt3->x, pt3->y) < 1e-4);
        assert(std::fabs(pt4->x - 50.0) < 1e-3);
        assert(std::fabs(pt4->y - 86.60254) < 1e-3);
        assert(std::fabs(cross_line) < 1e-4);
    }

    std::cout << "=== Все 9 ограничений и СЛАУ успешно прошли проверку! ===" << std::endl;
    return 0;
}
