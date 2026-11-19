#include "SketchImport.hpp"
#include "UseCases.hpp"
#include <emscripten.h>
#include <emscripten/bind.h>

using emscripten::val;

namespace {

cad::ApiHandler g_api;
std::string g_last_json;

} // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE
const char* api_get_all_primitives() {
    g_last_json = g_api.getAllPrimitives();
    return g_last_json.c_str();
}

EMSCRIPTEN_KEEPALIVE
int api_create_point(double x, double y) { return g_api.createPoint(x, y); }

EMSCRIPTEN_KEEPALIVE
int api_create_segment(int p1_id, int p2_id) { return g_api.createSegment(p1_id, p2_id); }

EMSCRIPTEN_KEEPALIVE
int api_delete_primitive(int id) { return g_api.deletePrimitive(id) ? 1 : 0; }

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
    return g_api.applyRestriction(static_cast<int>(cad::RestrictionType::POINT_ON_LINE),
                                  {p_id, lp1_id, lp2_id}, {seg_id}, 0.0, 0.0);
}

EMSCRIPTEN_KEEPALIVE
int api_remove_restriction(int rid) { return g_api.removeRestriction(rid) ? 1 : 0; }

EMSCRIPTEN_KEEPALIVE
void api_move_point(int point_id, double x, double y) { g_api.movePoint(point_id, x, y); }

EMSCRIPTEN_KEEPALIVE
void api_clear() { g_api.clearSketch(); }

} // extern "C"

EMSCRIPTEN_BINDINGS(editor_api) {
    using emscripten::function;
    function("getAllPrimitives", +[]() { return g_api.getAllPrimitives(); });
    function("serializeSketch", +[]() { return g_api.getRepository().serializeToJson(); });
    function("loadSketch", +[](const val& data) { return cad::wasm::loadSketch(g_api, data); });
    function(
        "createPoint",
        +[](double x, double y, const std::string& label) { return g_api.createPoint(x, y, label); });
    function(
        "createSegment",
        +[](int p1, int p2, const std::string& label) { return g_api.createSegment(p1, p2, label); });
    function(
        "createSegmentAt", +[](double x1, double y1, double x2, double y2, int snap_start, int snap_end,
                               const std::string& label) {
            return g_api.createSegmentAt(x1, y1, x2, y2, snap_start, snap_end, label);
        });
    function("deletePrimitive", +[](int id) { return g_api.deletePrimitive(id); });
    function(
        "applyRestriction",
        +[](int type, const val& points, const val& segments, double value, double value2) {
            return g_api.applyRestriction(type, cad::wasm::readIds(points), cad::wasm::readIds(segments),
                                          value, value2);
        });
    function(
        "updateRestriction",
        +[](int id, double value, double value2) { return g_api.updateRestriction(id, value, value2); });
    function("removeRestriction", +[](int id) { return g_api.removeRestriction(id); });
    function(
        "queryPoint", +[](double x, double y, double radius, const val& excluded) {
            return g_api.queryPoint(x, y, radius, cad::wasm::readIds(excluded));
        });
    function(
        "querySegment", +[](double x, double y, double radius) { return g_api.querySegment(x, y, radius); });
    function("movePoint", &api_move_point);
    function("moveSegment", +[](int id, double dx, double dy) { g_api.moveSegment(id, dx, dy); });
    function("solveSketch", +[]() { g_api.solveSketch(); });
    function("clearSketch", &api_clear);
}
