#pragma once
#include "CoreAlgo.hpp"
#include "GeomRepository.hpp"

namespace cad {

/**
 * Geom Solver (соответствует блоку Geom Solver на схеме бригады):
 * solve(geom, restrictions, new_restrictions) -> new_geom
 */
class GeomSolver {
  public:
    static OptimizeResult solve(GeomRepository& repo, const std::vector<Restriction>& extra_restrictions = {},
                                const std::vector<int>& dragged_point_ids = {});
};

} // namespace cad
