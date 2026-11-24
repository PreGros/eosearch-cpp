#include "geosearch/indexer.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace geosearch {

namespace {

// 'replace' makes dump() substitute invalid UTF-8 instead of throwing.
std::string dump_utf8(const nlohmann::json& j) {
    return j.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}

nlohmann::json to_document(const Poi& poi) {
    nlohmann::json doc;
    doc["osm_id"] = poi.osm_id;
    doc["name"] = poi.name;
    doc["category"] = poi.category;
    doc["location"]["lat"] = poi.lat;
    doc["location"]["lon"] = poi.lon;
    if (poi.city) doc["city"] = *poi.city;
    if (poi.street) doc["street"] = *poi.street;
    if (poi.housenumber) doc["housenumber"] = *poi.housenumber;
    return doc;
}

// Builds the NDJSON body: an action line, then the document line, for each POI.
std::string build_bulk_body(const std::vector<Poi>& pois,
                            std::size_t begin,
                            std::size_t end,
                            const std::string& index) {
    std::string body;
    for (std::size_t i = begin; i < end; ++i) {
        nlohmann::json action;
        action["index"]["_index"] = index;
        action["index"]["_id"] = pois[i].osm_id;

        body += dump_utf8(action);
        body += '\n';
        body += dump_utf8(to_document(pois[i]));
        body += '\n';
    }
    return body;
}

}  // namespace

IndexStats bulk_index(const std::string& es_url,
                      const std::string& index,
                      const std::vector<Poi>& pois,
                      std::size_t batch_size) {
    IndexStats stats;
    std::size_t errors_printed = 0;

    for (std::size_t begin = 0; begin < pois.size(); begin += batch_size) {
        const std::size_t end = std::min(begin + batch_size, pois.size());

        const cpr::Response r = cpr::Post(
            cpr::Url{es_url + "/_bulk"},
            cpr::Header{{"Content-Type", "application/x-ndjson"}},
            cpr::Body{build_bulk_body(pois, begin, end, index)},
            cpr::Timeout{60000});

        if (r.error) {
            throw std::runtime_error("Bulk request failed: " + r.error.message);
        }
        if (r.status_code != 200) {
            throw std::runtime_error("Bulk request returned HTTP " +
                                     std::to_string(r.status_code) + ": " + r.text);
        }

        // HTTP 200 does not mean every document was accepted, checking each item
        const auto response = nlohmann::json::parse(r.text);
        for (const auto& item : response.at("items")) {
            const auto& result = item.begin().value();
            if (result.contains("error")) {
                ++stats.failed;
                if (errors_printed < 5) { // output first 5 errors
                    ++errors_printed;
                    std::cerr << "Failed: " << result.value("_id", std::string{"?"}) << " -> "
                              << result.at("error").dump() << '\n';
                }
            } else {
                ++stats.indexed;
            }
        }
        std::cout << "  batch " << begin << "-" << end << " sent\n";
    }

    // Make the new documents visible to search right away
    cpr::Post(cpr::Url{es_url + "/" + index + "/_refresh"}, cpr::Timeout{10000});
    return stats;
}

std::size_t count_documents(const std::string& es_url, const std::string& index) {
    const cpr::Response r =
        cpr::Get(cpr::Url{es_url + "/" + index + "/_count"}, cpr::Timeout{10000});
    if (r.error || r.status_code != 200) {
        throw std::runtime_error("Count request failed: " + r.text);
    }
    return nlohmann::json::parse(r.text).at("count").get<std::size_t>();
}

}  // namespace geosearch