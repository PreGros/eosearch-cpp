#include "geosearch/poi.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <utility>

namespace geosearch {

namespace {

// Returns the tag value if the tag exists and is a string.
std::optional<std::string> tag(const nlohmann::json& tags, const char* key) {
    const auto it = tags.find(key);
    if (it == tags.end() || !it->is_string()) {
        return std::nullopt;
    }
    return it->get<std::string>();
}

// Czech addresses often use "conscription number / street number" (e.g. 12/3)
// instead of plain addr:housenumber.
std::optional<std::string> housenumber(const nlohmann::json& tags) {
    if (auto h = tag(tags, "addr:housenumber")) {
        return h;
    }
    const auto conscription = tag(tags, "addr:conscriptionnumber");
    const auto street_number = tag(tags, "addr:streetnumber");
    if (conscription && street_number) {
        return *conscription + "/" + *street_number;
    }
    if (conscription) {
        return conscription;
    }
    return street_number;
}

}  // namespace

std::optional<Poi> poi_from_osm_element(const nlohmann::json& element) {
    if (!element.is_object() || !element.contains("id") || !element.contains("tags")) {
        return std::nullopt;
    }
    const auto& tags = element.at("tags");

    auto name = tag(tags, "name");
    if (!name) {
        return std::nullopt;  // a POI without a name is useless for search
    }

    std::optional<std::string> category = tag(tags, "amenity");
    if (!category) category = tag(tags, "shop");
    if (!category) category = tag(tags, "tourism");
    if (!category) {
        return std::nullopt;
    }

    // Nodes carry lat/lon directly; ways and relations carry them under "center".
    const nlohmann::json* position = nullptr;
    if (element.contains("lat") && element.contains("lon")) {
        position = &element;
    } else if (element.contains("center")) {
        position = &element.at("center");
    }
    if (position == nullptr || !position->contains("lat") || !position->contains("lon")) {
        return std::nullopt;
    }

    Poi poi;
    poi.osm_id = element.value("type", std::string{"unknown"}) + "/" +
                 std::to_string(element.at("id").get<long long>());
    poi.name = std::move(*name);
    poi.category = std::move(*category);
    poi.city = tag(tags, "addr:city");
    poi.street = tag(tags, "addr:street");
    poi.housenumber = housenumber(tags);
    poi.lat = position->at("lat").get<double>();
    poi.lon = position->at("lon").get<double>();
    return poi;
}

std::vector<Poi> load_pois(const std::string& path, const std::string& default_city) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    const auto doc = nlohmann::json::parse(in);

    std::vector<Poi> result;
    for (const auto& element : doc.at("elements")) {
        if (auto poi = poi_from_osm_element(element)) {
            if (!poi->city) {
                poi->city = default_city;
            }
            result.push_back(std::move(*poi));
        }
    }
    return result;
}

}  // namespace geosearch