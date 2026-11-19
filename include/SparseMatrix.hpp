#pragma once
#include <map>
#include <vector>

namespace cad {

using SparseRow = std::map<int, double>;
using SparseMatrix = std::vector<SparseRow>;

} // namespace cad
