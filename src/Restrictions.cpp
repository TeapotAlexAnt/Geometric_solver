#include "Restrictions.hpp"
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace cad {

int Restriction::equationCount() const {
    if (type == RestrictionType::COINCIDENT || type == RestrictionType::FIX_POINT) {
        return 2;
    }
    return 1;
}

std::string Restriction::typeName() const {
    switch (type) {
    case RestrictionType::COINCIDENT:
        return "COINCIDENT";
    case RestrictionType::DISTANCE:
        return "DISTANCE";
    case RestrictionType::PARALLEL:
        return "PARALLEL";
    case RestrictionType::PERPENDICULAR:
        return "PERPENDICULAR";
    case RestrictionType::VERTICAL:
        return "VERTICAL";
    case RestrictionType::HORIZONTAL:
        return "HORIZONTAL";
    case RestrictionType::ANGLE:
        return "ANGLE";
    case RestrictionType::POINT_ON_LINE:
        return "POINT_ON_LINE";
    case RestrictionType::FIX_POINT:
        return "FIX_POINT";
    }
    return "UNKNOWN";
}

void Restriction::evaluateAnalytical(const std::vector<double>& X,
                                     const std::unordered_map<int, int>& pointIdToVarIdx,
                                     std::vector<double>& f_out, SparseMatrix& df_out,
                                     std::vector<std::map<int, SparseRow>>& d2f_out) const {
    int eqs = equationCount();
    f_out.assign(eqs, 0.0);
    df_out.assign(eqs, {});
    d2f_out.assign(eqs, {});

    auto idxX = [&](int pid) -> int { return pointIdToVarIdx.at(pid); };
    auto idxY = [&](int pid) -> int { return pointIdToVarIdx.at(pid) + 1; };

    switch (type) {
    case RestrictionType::COINCIDENT: {
        // Слайд 28, п. 1:
        // f1 = (x2 + dx2) - (x1 + dx1) = 0
        // f2 = (y2 + dy2) - (y1 + dy1) = 0
        int ix1 = idxX(p_ids[0]), iy1 = idxY(p_ids[0]);
        int ix2 = idxX(p_ids[1]), iy2 = idxY(p_ids[1]);

        f_out[0] = X[ix2] - X[ix1];
        df_out[0][ix1] -= 1.0;
        df_out[0][ix2] += 1.0;

        f_out[1] = X[iy2] - X[iy1];
        df_out[1][iy1] -= 1.0;
        df_out[1][iy2] += 1.0;
        break;
    }

    case RestrictionType::DISTANCE: {
        // Слайд 28, п. 2 и слайд 31:
        // f = (X2 - X1)^2 + (Y2 - Y1)^2 - d^2 = 0
        int ix1 = idxX(p_ids[0]), iy1 = idxY(p_ids[0]);
        int ix2 = idxX(p_ids[1]), iy2 = idxY(p_ids[1]);
        double dx = X[ix2] - X[ix1];
        double dy = X[iy2] - X[iy1];
        double d = value;

        f_out[0] = dx * dx + dy * dy - d * d;

        // Аналитические первые производные:
        df_out[0][ix1] -= 2.0 * dx;
        df_out[0][iy1] -= 2.0 * dy;
        df_out[0][ix2] += 2.0 * dx;
        df_out[0][iy2] += 2.0 * dy;

        // Аналитические вторые производные (матрица Якоби для системы Лагранжа (**)):
        d2f_out[0][ix1][ix1] += 2.0;
        d2f_out[0][ix2][ix2] += 2.0;
        d2f_out[0][ix1][ix2] += -2.0;
        d2f_out[0][ix2][ix1] += -2.0;

        d2f_out[0][iy1][iy1] += 2.0;
        d2f_out[0][iy2][iy2] += 2.0;
        d2f_out[0][iy1][iy2] += -2.0;
        d2f_out[0][iy2][iy1] += -2.0;
        break;
    }

    case RestrictionType::PARALLEL: {
        // Слайд 28, п. 3:
        // v12 x v34 = (X2 - X1)*(Y4 - Y3) - (X4 - X3)*(Y2 - Y1) = 0
        int ix1 = idxX(p_ids[0]), iy1 = idxY(p_ids[0]);
        int ix2 = idxX(p_ids[1]), iy2 = idxY(p_ids[1]);
        int ix3 = idxX(p_ids[2]), iy3 = idxY(p_ids[2]);
        int ix4 = idxX(p_ids[3]), iy4 = idxY(p_ids[3]);

        double ux = X[ix2] - X[ix1];
        double uy = X[iy2] - X[iy1];
        double vx = X[ix4] - X[ix3];
        double vy = X[iy4] - X[iy3];

        f_out[0] = ux * vy - vx * uy;

        // Аналитические первые производные (+= на случай общих вершин у двух отрезков):
        df_out[0][ix1] += -vy;
        df_out[0][iy1] += vx;
        df_out[0][ix2] += vy;
        df_out[0][iy2] += -vx;
        df_out[0][ix3] += uy;
        df_out[0][iy3] += -ux;
        df_out[0][ix4] += -uy;
        df_out[0][iy4] += ux;

        // Аналитические вторые производные:
        auto addSym = [&](int r, int c, double val) {
            d2f_out[0][r][c] += val;
            d2f_out[0][c][r] += val;
        };
        addSym(ix1, iy3, 1.0);
        addSym(ix1, iy4, -1.0);
        addSym(ix2, iy3, -1.0);
        addSym(ix2, iy4, 1.0);

        addSym(iy1, ix3, -1.0);
        addSym(iy1, ix4, 1.0);
        addSym(iy2, ix3, 1.0);
        addSym(iy2, ix4, -1.0);
        break;
    }

    case RestrictionType::PERPENDICULAR: {
        // Слайд 28, п. 4:
        // (v12 . v34) = (X2 - X1)*(X4 - X3) + (Y2 - Y1)*(Y4 - Y3) = 0
        int ix1 = idxX(p_ids[0]), iy1 = idxY(p_ids[0]);
        int ix2 = idxX(p_ids[1]), iy2 = idxY(p_ids[1]);
        int ix3 = idxX(p_ids[2]), iy3 = idxY(p_ids[2]);
        int ix4 = idxX(p_ids[3]), iy4 = idxY(p_ids[3]);

        double ux = X[ix2] - X[ix1];
        double uy = X[iy2] - X[iy1];
        double vx = X[ix4] - X[ix3];
        double vy = X[iy4] - X[iy3];

        f_out[0] = ux * vx + uy * vy;

        // Аналитические первые производные:
        df_out[0][ix1] += -vx;
        df_out[0][iy1] += -vy;
        df_out[0][ix2] += vx;
        df_out[0][iy2] += vy;
        df_out[0][ix3] += -ux;
        df_out[0][iy3] += -uy;
        df_out[0][ix4] += ux;
        df_out[0][iy4] += uy;

        // Аналитические вторые производные:
        auto addSym = [&](int r, int c, double val) {
            d2f_out[0][r][c] += val;
            d2f_out[0][c][r] += val;
        };
        addSym(ix1, ix3, 1.0);
        addSym(ix1, ix4, -1.0);
        addSym(ix2, ix3, -1.0);
        addSym(ix2, ix4, 1.0);

        addSym(iy1, iy3, 1.0);
        addSym(iy1, iy4, -1.0);
        addSym(iy2, iy3, -1.0);
        addSym(iy2, iy4, 1.0);
        break;
    }

    case RestrictionType::VERTICAL: {
        // Слайд 29, п. 5:
        // f = (x2 + dx2) - (x1 + dx1) = 0
        int ix1 = idxX(p_ids[0]);
        int ix2 = idxX(p_ids[1]);
        f_out[0] = X[ix2] - X[ix1];
        df_out[0][ix1] -= 1.0;
        df_out[0][ix2] += 1.0;
        break;
    }

    case RestrictionType::HORIZONTAL: {
        // Слайд 29, п. 6:
        // f = (y2 + dy2) - (y1 + dy1) = 0
        int iy1 = idxY(p_ids[0]);
        int iy2 = idxY(p_ids[1]);
        f_out[0] = X[iy2] - X[iy1];
        df_out[0][iy1] -= 1.0;
        df_out[0][iy2] += 1.0;
        break;
    }

    case RestrictionType::ANGLE: {
        // Слайд 29, п. 7:
        // (v12 . v34) - |v12||v34| cos(alpha) = 0
        int ix1 = idxX(p_ids[0]), iy1 = idxY(p_ids[0]);
        int ix2 = idxX(p_ids[1]), iy2 = idxY(p_ids[1]);
        int ix3 = idxX(p_ids[2]), iy3 = idxY(p_ids[2]);
        int ix4 = idxX(p_ids[3]), iy4 = idxY(p_ids[3]);

        double ux = X[ix2] - X[ix1];
        double uy = X[iy2] - X[iy1];
        double vx = X[ix4] - X[ix3];
        double vy = X[iy4] - X[iy3];

        double L1 = std::hypot(ux, uy);
        double L2 = std::hypot(vx, vy);
        if (L1 < 1e-9) L1 = 1e-9;
        if (L2 < 1e-9) L2 = 1e-9;

        constexpr double kPi = 3.14159265358979323846;
        double alpha_rad = value * kPi / 180.0;
        double c = std::cos(alpha_rad);

        f_out[0] = (ux * vx + uy * vy) - L1 * L2 * c;

        // Аналитические первые производные по компонентам векторов u и v:
        // df/dux = vx - (ux / L1) * L2 * cos(alpha)
        // df/duy = vy - (uy / L1) * L2 * cos(alpha)
        // df/dvx = ux - L1 * (vx / L2) * cos(alpha)
        // df/dvy = uy - L1 * (vy / L2) * cos(alpha)
        double df_dux = vx - (ux / L1) * L2 * c;
        double df_duy = vy - (uy / L1) * L2 * c;
        double df_dvx = ux - L1 * (vx / L2) * c;
        double df_dvy = uy - L1 * (vy / L2) * c;

        df_out[0][ix1] += -df_dux;
        df_out[0][iy1] += -df_duy;
        df_out[0][ix2] += df_dux;
        df_out[0][iy2] += df_duy;
        df_out[0][ix3] += -df_dvx;
        df_out[0][iy3] += -df_dvy;
        df_out[0][ix4] += df_dvx;
        df_out[0][iy4] += df_dvy;

        // Аналитические вторые производные по u, v:
        double L1_3 = L1 * L1 * L1;
        double L2_3 = L2 * L2 * L2;
        double d2_ux_ux = -c * L2 * (uy * uy) / L1_3;
        double d2_uy_uy = -c * L2 * (ux * ux) / L1_3;
        double d2_ux_uy = c * L2 * (ux * uy) / L1_3;

        double d2_vx_vx = -c * L1 * (vy * vy) / L2_3;
        double d2_vy_vy = -c * L1 * (vx * vx) / L2_3;
        double d2_vx_vy = c * L1 * (vx * vy) / L2_3;

        double d2_ux_vx = 1.0 - c * (ux / L1) * (vx / L2);
        double d2_ux_vy = -c * (ux / L1) * (vy / L2);
        double d2_uy_vx = -c * (uy / L1) * (vx / L2);
        double d2_uy_vy = 1.0 - c * (uy / L1) * (vy / L2);

        int u_idx[4] = {ix1, iy1, ix2, iy2};
        double su[2] = {-1.0, 1.0};
        for (int a = 0; a < 2; ++a) {
            for (int b = 0; b < 2; ++b) {
                double s = su[a] * su[b];
                d2f_out[0][u_idx[a * 2 + 0]][u_idx[b * 2 + 0]] += s * d2_ux_ux;
                d2f_out[0][u_idx[a * 2 + 1]][u_idx[b * 2 + 1]] += s * d2_uy_uy;
                d2f_out[0][u_idx[a * 2 + 0]][u_idx[b * 2 + 1]] += s * d2_ux_uy;
                d2f_out[0][u_idx[a * 2 + 1]][u_idx[b * 2 + 0]] += s * d2_ux_uy;
            }
        }
        int v_idx[4] = {ix3, iy3, ix4, iy4};
        for (int a = 0; a < 2; ++a) {
            for (int b = 0; b < 2; ++b) {
                double s = su[a] * su[b];
                d2f_out[0][v_idx[a * 2 + 0]][v_idx[b * 2 + 0]] += s * d2_vx_vx;
                d2f_out[0][v_idx[a * 2 + 1]][v_idx[b * 2 + 1]] += s * d2_vy_vy;
                d2f_out[0][v_idx[a * 2 + 0]][v_idx[b * 2 + 1]] += s * d2_vx_vy;
                d2f_out[0][v_idx[a * 2 + 1]][v_idx[b * 2 + 0]] += s * d2_vx_vy;
            }
        }
        for (int a = 0; a < 2; ++a) {
            for (int b = 0; b < 2; ++b) {
                double s = su[a] * su[b];
                int ux_i = u_idx[a * 2 + 0], uy_i = u_idx[a * 2 + 1];
                int vx_i = v_idx[b * 2 + 0], vy_i = v_idx[b * 2 + 1];
                d2f_out[0][ux_i][vx_i] += s * d2_ux_vx;
                d2f_out[0][vx_i][ux_i] += s * d2_ux_vx;
                d2f_out[0][ux_i][vy_i] += s * d2_ux_vy;
                d2f_out[0][vy_i][ux_i] += s * d2_ux_vy;
                d2f_out[0][uy_i][vx_i] += s * d2_uy_vx;
                d2f_out[0][vx_i][uy_i] += s * d2_uy_vx;
                d2f_out[0][uy_i][vy_i] += s * d2_uy_vy;
                d2f_out[0][vy_i][uy_i] += s * d2_uy_vy;
            }
        }
        break;
    }

    case RestrictionType::POINT_ON_LINE: {
        // Слайд 29, п. 8:
        // v_{1P} x v_{P2} = 0 =>
        // f = (X_P - X_1)*(Y_2 - Y_P) - (X_2 - X_P)*(Y_P - Y_1) = 0
        int ixp = idxX(p_ids[0]), iyp = idxY(p_ids[0]);
        int ix1 = idxX(p_ids[1]), iy1 = idxY(p_ids[1]);
        int ix2 = idxX(p_ids[2]), iy2 = idxY(p_ids[2]);

        double xp = X[ixp], yp = X[iyp];
        double x1 = X[ix1], y1 = X[iy1];
        double x2 = X[ix2], y2 = X[iy2];

        f_out[0] = (xp - x1) * (y2 - yp) - (x2 - xp) * (yp - y1);

        // Аналитические первые производные:
        df_out[0][ixp] += (y2 - y1);
        df_out[0][iyp] += -(x2 - x1);
        df_out[0][ix1] += -(y2 - yp);
        df_out[0][iy1] += (x2 - xp);
        df_out[0][ix2] += -(yp - y1);
        df_out[0][iy2] += (xp - x1);

        // Аналитические вторые производные:
        auto addSym = [&](int r, int c, double val) {
            d2f_out[0][r][c] += val;
            d2f_out[0][c][r] += val;
        };
        addSym(ixp, iy1, -1.0);
        addSym(ixp, iy2, 1.0);
        addSym(iyp, ix1, 1.0);
        addSym(iyp, ix2, -1.0);
        addSym(ix1, iy2, -1.0);
        addSym(iy1, ix2, 1.0);
        break;
    }

    case RestrictionType::FIX_POINT: {
        // Слайд 29, п. 9:
        // f1 = x + dx - x_c = 0
        // f2 = y + dy - y_c = 0
        int ix = idxX(p_ids[0]), iy = idxY(p_ids[0]);
        f_out[0] = X[ix] - value;
        df_out[0][ix] = 1.0;

        f_out[1] = X[iy] - value2;
        df_out[1][iy] = 1.0;
        break;
    }
    }
    for (auto& gradient : df_out) {
        for (auto it = gradient.begin(); it != gradient.end();) {
            if (it->second == 0.0)
                it = gradient.erase(it);
            else
                ++it;
        }
    }
}

std::string Restriction::toJson() const {
    std::ostringstream ss;
    ss << std::setprecision(std::numeric_limits<double>::max_digits10);
    ss << "{\"id\":" << id << ",\"type\":\"" << typeName() << "\""
       << ",\"type_code\":" << static_cast<int>(type) << ",\"value\":" << value << ",\"value2\":" << value2
       << ",\"p_ids\":[";
    for (size_t i = 0; i < p_ids.size(); ++i) {
        if (i) ss << ",";
        ss << p_ids[i];
    }
    ss << "],\"seg_ids\":[";
    for (size_t i = 0; i < seg_ids.size(); ++i) {
        if (i) ss << ",";
        ss << seg_ids[i];
    }
    ss << "]}";
    return ss.str();
}

} // namespace cad
