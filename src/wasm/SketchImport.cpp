#include "SketchImport.hpp"
#include "UseCases.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_map>
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
        std::vector<std::pair<int, val>> segments;
        const auto addImportedPoint = [&](const val& point) {
            int id = registerId(point);
            int owner = point["type"].strictlyEquals(val("endpoint")) ? readId(point["segment_id"]) : 0;
            repo.points.push_back(std::make_shared<cad::PointFigure>(
                id, readNumber(point["x"]), readNumber(point["y"]), readLabel(point), owner));
        };

        // The browser previously exported points/segments. The C++ serializer uses primitives.
        if (data["primitives"].isUndefined()) {
            for (const auto& point : optionalArray(data["points"]))
                addImportedPoint(point);
            for (const auto& segment : optionalArray(data["segments"]))
                segments.emplace_back(registerId(segment), segment);
        } else {
            for (const auto& primitive : optionalArray(data["primitives"])) {
                if (primitive.isNull() || primitive.typeOf().as<std::string>() != "object") {
                    return "Ожидался объект примитива";
                }
                auto type = primitive["type"];
                if (type.strictlyEquals(val("point")) || type.strictlyEquals(val("endpoint")))
                    addImportedPoint(primitive);
                else if (type.strictlyEquals(val("line")))
                    segments.emplace_back(registerId(primitive), primitive);
                else
                    return "Неизвестный тип примитива";
            }
        }

        std::vector<cad::Restriction> restrictions;
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
            restrictions.push_back(std::move(r));
        }

        std::unordered_map<int, cad::PointFigure*> source_points;
        for (const auto& point : repo.points)
            source_points.emplace(point->id, point.get());
        std::unordered_set<int> used_points;
        std::unordered_set<int> legacy_points;
        std::vector<cad::Restriction> links;
        const auto claimEndpoint = [&](int point_id, int segment_id, const std::string& label) {
            auto source = source_points.find(point_id);
            if (source == source_points.end()) throw std::invalid_argument("Не найден конец отрезка");
            auto* point = source->second;
            if (used_points.insert(point_id).second) {
                if (!point->segment_id) {
                    legacy_points.insert(point_id);
                    point->segment_id = segment_id;
                    point->type = cad::FigureType::ENDPOINT;
                }
                if (point->segment_id != segment_id) {
                    throw std::invalid_argument("Конец принадлежит другому отрезку");
                }
                return point;
            }
            if (!legacy_points.count(point_id)) throw std::invalid_argument("Общий конец отрезков");
            if (repo.next_id == std::numeric_limits<int>::max()) {
                throw std::invalid_argument("Нет свободного идентификатора для конца отрезка");
            }
            auto copy =
                std::make_shared<cad::PointFigure>(repo.next_id++, point->x, point->y, label, segment_id);
            repo.points.push_back(copy);
            links.push_back({0, cad::RestrictionType::COINCIDENT, {point_id, copy->id}, {}, 0, 0});
            return copy.get();
        };

        std::unordered_map<int, std::array<int, 2>> original_ends;
        for (const auto& [id, item] : segments) {
            const int start_id = readId(item["p1_id"]);
            const int end_id = readId(item["p2_id"]);
            if (start_id == end_id) return "Некорректные концы отрезка";
            auto label = readLabel(item);
            if (label.empty()) label = "L" + std::to_string(id);
            auto* start = claimEndpoint(start_id, id, label + ".1");
            auto* end = claimEndpoint(end_id, id, label + ".2");
            repo.segments.push_back(
                std::make_shared<cad::SegmentFigure>(id, start->id, end->id, start, end, label));
            original_ends.emplace(id, std::array<int, 2>{start_id, end_id});
        }
        for (const auto& [id, point] : source_points) {
            if (point->segment_id && !used_points.count(id)) return "Конец без отрезка";
        }

        for (auto& r : restrictions) {
            if (!std::all_of(r.p_ids.begin(), r.p_ids.end(),
                             [&](int id) { return source_points.count(id); }) ||
                !repo.isValidRestriction(r)) {
                return "Некорректные объекты ограничения";
            }
            // Ограничение отрезка должно ссылаться на его собственные концы после миграции.
            const auto remapPair = [&](size_t segment_index, size_t offset) {
                if (segment_index >= r.seg_ids.size()) return;
                int id = r.seg_ids[segment_index];
                const auto& source = original_ends.at(id);
                const auto* segment = repo.findSegment(id);
                auto& a = r.p_ids[offset];
                auto& b = r.p_ids[offset + 1];
                if (a == source[0] && b == source[1]) {
                    a = segment->p1_id;
                    b = segment->p2_id;
                } else if (a == source[1] && b == source[0]) {
                    a = segment->p2_id;
                    b = segment->p1_id;
                }
            };
            switch (r.type) {
            case cad::RestrictionType::DISTANCE:
            case cad::RestrictionType::HORIZONTAL:
            case cad::RestrictionType::VERTICAL:
                remapPair(0, 0);
                break;
            case cad::RestrictionType::PARALLEL:
            case cad::RestrictionType::PERPENDICULAR:
            case cad::RestrictionType::ANGLE:
                remapPair(0, 0);
                remapPair(1, 2);
                break;
            case cad::RestrictionType::POINT_ON_LINE:
                remapPair(0, 1);
                break;
            default:
                break;
            }
            repo.addRestriction(r);
        }
        for (const auto& link : links) {
            if (repo.next_id == std::numeric_limits<int>::max()) {
                return "Нет свободного идентификатора для ограничения совпадения";
            }
            repo.addRestriction(link);
        }

        api.replaceSketch(std::move(repo));
        return "";
    } catch (const std::exception& error) {
        return error.what();
    }
}

} // namespace cad::wasm
