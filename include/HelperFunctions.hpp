#pragma once
#include "GeomRepository.hpp"
#include <vector>

namespace cad {

/**
 * Helper Functions -> query (соответствует блоку Helper Functions / query на схеме бригады):
 * Поиск объектов по координатам курсора, проверка связности и подсчет степеней свободы (DOF).
 */
class HelperFunctions {
  public:
    static int queryPointAt(const GeomRepository& repo, double x, double y, double radius = 10.0,
                            const std::vector<int>& excluded_ids = {});

    static int querySegmentAt(const GeomRepository& repo, double x, double y, double tolerance = 8.0);

    static int countEquations(const GeomRepository& repo);

    static std::vector<std::vector<const PointFigure*>>
    groupCoincidentPoints(const GeomRepository& repo,
                          const std::vector<Restriction>& extra_restrictions = {});

    static int computeDOF(const GeomRepository& repo);
};

} // namespace cad
