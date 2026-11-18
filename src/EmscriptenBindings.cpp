#include "../include/UseCases.hpp"
#include <string>

static cad::ApiHandler g_api;
static std::string g_last_json;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>

extern "C" {

EMSCRIPTEN_KEEPALIVE
const char* api_get_all_primitives() {
    g_last_json = g_api.getAllPrimitives();
    return g_last_json.c_str();
}

EMSCRIPTEN_KEEPALIVE
int api_create_point(double x, double y) {
    return g_api.createPoint(x, y);
}

EMSCRIPTEN_KEEPALIVE
int api_create_segment(int p1_id, int p2_id) {
    return g_api.createSegment(p1_id, p2_id);
}

EMSCRIPTEN_KEEPALIVE
int api_delete_primitive(int id) {
    return g_api.deletePrimitive(id) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int api_add_restriction_2p(int type_code, int p1, int p2, double val1, double val2) {
    std::vector<int> pids;
    if (p1 > 0) pids.push_back(p1);
    if (p2 > 0) pids.push_back(p2);
    return g_api.applyRestriction(type_code, pids, {}, val1, val2);
}

EMSCRIPTEN_KEEPALIVE
int api_add_restriction_4p(int type_code, int p1, int p2, int p3, int p4, int s1, int s2, double val1) {
    std::vector<int> pids = {p1, p2, p3, p4};
    std::vector<int> sids = {s1, s2};
    return g_api.applyRestriction(type_code, pids, sids, val1, 0.0);
}

EMSCRIPTEN_KEEPALIVE
int api_add_restriction_point_on_line(int p_id, int lp1_id, int lp2_id, int seg_id) {
    return g_api.applyRestriction(
        static_cast<int>(cad::RestrictionType::POINT_ON_LINE),
        {p_id, lp1_id, lp2_id},
        {seg_id},
        0.0, 0.0
    );
}

EMSCRIPTEN_KEEPALIVE
int api_remove_restriction(int rid) {
    return g_api.removeRestriction(rid) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
void api_move_point(int point_id, double x, double y) {
    g_api.movePoint(point_id, x, y);
}

EMSCRIPTEN_KEEPALIVE
void api_clear() {
    g_api.getRepository().clear();
}

} // extern "C"
#endif
