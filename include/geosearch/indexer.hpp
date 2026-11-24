#pragma once

#include "geosearch/poi.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace geosearch {

struct IndexStats {
    std::size_t indexed = 0;
    std::size_t failed = 0;
};

// Sends all POIs to Elasticsearch through the _bulk API, batch_size at a time.
// Uses the OSM id as document _id, so running it twice does not create duplicates.
// Calls _refresh at the end so the documents are searchable immediately.
// Throws std::runtime_error when a whole request fails (network, HTTP error).
// Failures of single documents are counted in IndexStats and the first few are printed.
IndexStats bulk_index(const std::string& es_url,
                      const std::string& index,
                      const std::vector<Poi>& pois,
                      std::size_t batch_size = 1000);

// Number of documents in the index.
std::size_t count_documents(const std::string& es_url, const std::string& index);

}  // namespace geosearch