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

    // Совпадение объединяет координаты транзитивно, сохраняя собственные идентификаторы концов.
    {
        ApiHandler api;
        auto& repo = api.getRepository();
        const int first_id = api.createSegmentAt(0, 0, 10, 3);
        const int second_id = api.createSegmentAt(10, 3, 20, 5);
        const int standalone = api.createPoint(10, 3);
        const auto* first = repo.findSegment(first_id);
        const auto* second = repo.findSegment(second_id);
        assert(first && second);
        const auto* a = first->p2_ref;
        const auto* b = second->p1_ref;
        const auto* c = repo.findPoint(standalone);
        assert(a->id != b->id && a->type == FigureType::ENDPOINT && b->type == FigureType::ENDPOINT);
        assert(a->segment_id == first_id && b->segment_id == second_id);
        assert(c->type == FigureType::POINT);
        assert(api.queryPoint(10, 3, 0.1, {b->id, c->id}) == a->id);
        assert(api.getLastSolveResult().final_params.size() == 10);

        const auto coincidence = [&](int p1, int p2) {
            return api.applyRestriction(static_cast<int>(RestrictionType::COINCIDENT), {p1, p2}, {}, 0);
        };
        const int horizontal =
            api.applyRestriction(static_cast<int>(RestrictionType::HORIZONTAL), {a->id, b->id}, {}, 0);
        assert(api.getLastSolveResult().last_jacobian.size() == 11);
        const int ab = coincidence(a->id, b->id);
        const int bc = coincidence(b->id, c->id);
        const int ca = coincidence(c->id, a->id);
        const auto& reduced = api.getLastSolveResult();
        assert(reduced.final_params.size() == 6);
        assert(reduced.last_jacobian.size() == 7);
        assert(reduced.final_residuals.size() == 1 && reduced.final_gradients[0].empty());
        assert(!reduced.last_jacobian[0].empty()); // диагональный элемент регуляризации

        const bool removed_horizontal = api.removeRestriction(horizontal);
        assert(removed_horizontal);
        api.movePoint(b->id, 30, 6);
        assert(a->x == 30 && a->y == 6 && c->x == a->x && c->y == a->y);
        assert(first->p1_ref->x == 0 && second->p2_ref->x == 20);

        const bool removed_ca = api.removeRestriction(ca);
        assert(removed_ca);
        assert(api.getLastSolveResult().final_params.size() == 6);
        const bool removed_bc = api.removeRestriction(bc);
        assert(removed_bc);
        assert(api.getLastSolveResult().final_params.size() == 8);
        api.moveSegment(second_id, 2, 1);
        assert(a->x == 32 && b->x == 32 && a->y == 7 && b->y == 7);
        assert(c->x == 30 && c->y == 6);

        const bool removed_ab = api.removeRestriction(ab);
        assert(removed_ab);
        assert(api.getLastSolveResult().final_params.size() == 10);
        api.movePoint(a->id, 8, 4);
        assert(a->x == 8 && a->y == 4 && b->x == 32 && b->y == 7);

        coincidence(a->id, c->id);
        const int deleted_end = a->id;
        const int deleted_start = first->p1_id;
        const bool deleted = api.deletePrimitive(deleted_end);
        assert(deleted);
        assert(!repo.findSegment(first_id) && !repo.findPoint(deleted_start) && !repo.findPoint(deleted_end));
        assert(repo.findSegment(second_id) && repo.findPoint(standalone) && repo.restrictions.empty());

        const bool batch_deleted = api.deletePrimitives({b->id, second_id, standalone});
        assert(batch_deleted);
        assert(repo.segments.empty() && repo.points.empty() && api.getLastSolveResult().final_params.empty());
    }

    {
        ApiHandler api;
        const int point = api.createPoint(5, 5);
        const int inner = api.createSegmentAt(1, 1, 8, 8);
        const int through = api.createSegmentAt(-5, 5, 15, 5);
        const int tangent = api.createSegmentAt(-5, 5, 5, -5);
        api.createSegmentAt(-5, 4, 4, -5); // Bounding boxes overlap but the line misses the window.
        assert(api.queryBox(0, 10, 10, 0) == std::vector<int>({point, inner}));
        assert(api.queryBox(10, 0, 0, 10) == std::vector<int>({point, inner, through, tangent}));
    }

    // Принадлежность прямой с тремя свободными точками даёт ортогональную проекцию
    // исходных точек на итоговую прямую.
    {
        ApiHandler api;
        const int a = api.createPoint(0, 0), b = api.createPoint(10, 0), p = api.createPoint(3, 20);
        api.applyRestriction(static_cast<int>(RestrictionType::POINT_ON_LINE), {p, a, b}, {}, 0);
        const auto& repo = api.getRepository();
        const auto* pa = repo.findPoint(a);
        const auto* pb = repo.findPoint(b);
        const auto* pp = repo.findPoint(p);
        const double dx = pb->x - pa->x, dy = pb->y - pa->y;
        assert(std::hypot(dx, dy) > 1);
        assert(api.getLastSolveResult().max_residual < 1e-6);
        assert(std::fabs(pa->x * dx + pa->y * dy) < 1e-5);
        assert(std::fabs((pb->x - 10) * dx + pb->y * dy) < 1e-5);
        assert(std::fabs((pp->x - 3) * dx + (pp->y - 20) * dy) < 1e-5);
    }

    // Цель около центра окружности не должна менять направление радиуса скачком.
    {
        ApiHandler api;
        const int a = api.createPoint(0, 0), b = api.createPoint(100, 0);
        api.applyRestriction(static_cast<int>(RestrictionType::FIX_POINT), {a}, {}, 0);
        api.applyRestriction(static_cast<int>(RestrictionType::DISTANCE), {a, b}, {}, 100);
        const auto* point = api.getRepository().findPoint(b);
        api.movePoint(b, 0, 1e-9);
        assert(std::hypot(point->x - 100, point->y) < 1e-5);

        api.movePoint(b, 120, 120);
        const double diagonal = 100 / std::sqrt(2.0);
        assert(std::hypot(point->x - diagonal, point->y - diagonal) < 1e-4);
        const double last_x = point->x, last_y = point->y;
        api.applyRestriction(static_cast<int>(RestrictionType::FIX_POINT), {b}, {}, 200, 200);
        assert(!api.getLastSolveResult().converged);
        assert(point->x == last_x && point->y == last_y);
    }

    // Пауза допускает независимое движение даже совпадающих и фиксированных точек.
    {
        ApiHandler api;
        const int pin_id = api.createPoint(0, 0);
        const int segment_id = api.createSegmentAt(0, 0, 10, 0, pin_id);
        const auto& repo = api.getRepository();
        const auto* pin = repo.findPoint(pin_id);
        const auto* segment = repo.findSegment(segment_id);
        const auto* start = segment->p1_ref;
        const auto* end = segment->p2_ref;
        api.applyRestriction(static_cast<int>(RestrictionType::FIX_POINT), {pin_id}, {}, 0);
        const int length = api.applyRestriction(static_cast<int>(RestrictionType::DISTANCE),
                                                {start->id, end->id}, {segment_id}, 10);

        api.setAutoSolve(false);
        api.movePoint(pin_id, 25, 7);
        assert(pin->x == 25 && pin->y == 7 && start->x == 0 && start->y == 0);
        api.moveSegment(segment_id, 4, 3);
        const bool updated = api.updateRestriction(length, 12, 0);
        assert(updated && repo.restrictions.size() == 3);
        assert(start->x == 4 && start->y == 3 && end->x == 14 && end->y == 3);
        assert(api.getLastSolveResult().last_jacobian.empty());

        api.solveSketch();
        assert(api.getLastSolveResult().converged);
        assert(pin->x == start->x && pin->y == start->y && std::hypot(pin->x, pin->y) < 1e-6);
        assert(std::fabs(std::hypot(end->x - start->x, end->y - start->y) - 12) < 1e-6);

        api.movePoint(start->id, 3, 4);
        assert(start->x == 3 && start->y == 4 && std::hypot(pin->x, pin->y) < 1e-6);
        api.setAutoSolve(true);
        assert(api.getLastSolveResult().converged);
        assert(pin->x == start->x && pin->y == start->y && std::hypot(pin->x, pin->y) < 1e-6);
    }

    std::cout << "=== Все 9 ограничений и СЛАУ успешно прошли проверку! ===" << std::endl;
    return 0;
}
