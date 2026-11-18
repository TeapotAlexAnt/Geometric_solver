#include "SketchImport.hpp"
#include "UseCases.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace cad::wasm {

using emscripten::val;

namespace {

double readNumber(const val& value) {
    if (value.isNumber()) {
        double number = value.as<double>();
        if (std::isfinite(number)) return number;
    }
    throw std::invalid_argument("Ожидалось конечное число");
}

int readId(const val& value) {
    double number = readNumber(value);
    if (number < 1 || number >= std::numeric_limits<int>::max() || std::trunc(number) != number) {
        throw std::invalid_argument("Некорректный идентификатор");
    }
    return static_cast<int>(number);
}

val optionalArray(const val& value) {
    if (value.isUndefined()) return val::array();
    if (!value.isArray()) throw std::invalid_argument("Ожидался массив");
    return value;
}

std::string readLabel(const val& object) {
    auto label = object["label"];
    return label.isString() ? label.as<std::string>() : "";
}

} // namespace

std::vector<int> readIds(const val& value) {
    std::vector<int> ids;
    const auto array = optionalArray(value);
    const auto length = array["length"].as<unsigned>();
    for (unsigned i = 0; i < length; ++i)
        ids.push_back(readId(array[i]));
    return ids;
}

std::string loadSketch(ApiHandler& api, const val& data) {
    try {
        if (data.typeOf().as<std::string>() != "object" || data.isNull() || data.isArray()) {
            return "Ожидался объект эскиза";
        }
        cad::GeomRepository repo;
        std::unordered_set<int> ids;
        const auto registerId = [&](const val& item) {
            if (item.isNull() || item.typeOf().as<std::string>() != "object") {
                throw std::invalid_argument("Ожидался объект геометрии");
            }
            int id = readId(item["id"]);
            if (!ids.insert(id).second) throw std::invalid_argument("Повторяющийся идентификатор");
            repo.next_id = std::max(repo.next_id, id + 1);
            return id;
        };
        const auto addImportedPoint = [&](const val& point) {
            int id = registerId(point);
            repo.points.push_back(std::make_shared<cad::PointFigure>(
                id, readNumber(point["x"]), readNumber(point["y"]), readLabel(point)));
        };
        const auto addImportedSegment = [&](const val& segment) {
            int id = registerId(segment);
            int p1 = readId(segment["p1_id"]);
            int p2 = readId(segment["p2_id"]);
            auto* first = repo.findPoint(p1);
            auto* second = repo.findPoint(p2);
            if (!first || !second || p1 == p2) throw std::invalid_argument("Некорректные концы отрезка");
            repo.segments.push_back(
                std::make_shared<cad::SegmentFigure>(id, p1, p2, first, second, readLabel(segment)));
        };

        // The browser previously exported points/segments. The C++ serializer uses primitives.
        if (data["primitives"].isUndefined()) {
            for (const auto& point : optionalArray(data["points"]))
                addImportedPoint(point);
            for (const auto& segment : optionalArray(data["segments"]))
                addImportedSegment(segment);
        } else {
            auto primitives = optionalArray(data["primitives"]);
            std::vector<val> segments;
            for (const auto& primitive : primitives) {
                if (primitive.isNull() || primitive.typeOf().as<std::string>() != "object") {
                    return "Ожидался объект примитива";
                }
                auto type = primitive["type"];
                if (type.strictlyEquals(val("point")))
                    addImportedPoint(primitive);
                else if (type.strictlyEquals(val("line")))
                    segments.push_back(primitive);
                else
                    return "Неизвестный тип примитива";
            }
            for (const auto& segment : segments)
                addImportedSegment(segment);
        }

        for (const auto& item : optionalArray(data["restrictions"])) {
            cad::Restriction r{};
            r.id = registerId(item);
            auto type = item["type"];
            if (!type.isString()) return "Ожидался тип ограничения";
            auto typeName = type.as<std::string>();
            for (int code = 1; code <= 9; ++code) {
                r.type = static_cast<cad::RestrictionType>(code);
                if (r.typeName() == typeName) break;
            }
            if (r.typeName() != typeName) return "Неизвестный тип ограничения";
            r.p_ids = readIds(item["p_ids"]);
            r.seg_ids = readIds(item["seg_ids"]);
            r.value = item["value"].isUndefined() ? 0.0 : readNumber(item["value"]);
            r.value2 = item["value2"].isUndefined() ? 0.0 : readNumber(item["value2"]);
            if (!repo.isValidRestriction(r)) return "Некорректные объекты ограничения";
            repo.addRestriction(r);
        }

        api.replaceSketch(std::move(repo));
        return "";
    } catch (const std::exception& error) {
        return error.what();
    }
}

} // namespace cad::wasm
