#pragma once

#include <nlohmann/json_fwd.hpp>

#include <optional>
#include <string>
#include <vector>

namespace geosearch {

struct Poi {
    std::string osm_id;
    std::string name;
    std::string category;
    std::optional<std::string> city;
    std::optional<std::string> street;
    std::optional<std::string> housenumber;
    double lat = 0.0;
    double lon = 0.0;
};

// Converts one element from the Overpass JSON into a Poi.
// Returns std::nullopt when the element is unusable (no name, no category,
// no id or no coordinates).
std::optional<Poi> poi_from_osm_element(const nlohmann::json& element);

// Reads an Overpass JSON file and returns all usable POIs.
// POIs without an addr:city tag get default_city (we queried one city only).
// Throws std::runtime_error if the file cannot be opened.
std::vector<Poi> load_pois(const std::string& path, const std::string& default_city);

}  // namespace geosearch
