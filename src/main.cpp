#include "geosearch/indexer.hpp"
#include "geosearch/poi.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

std::string es_url() {
    if (const char* v = std::getenv("ES_URL")) {
        return v;
    }
    return "http://localhost:9200";
}

bool ping_elasticsearch() {
    const std::string url = es_url();
    const cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Timeout{5000});
    if (r.error || r.status_code != 200) {
        std::cerr << "Elasticsearch not reachable at " << url
                  << ". Is the container running? (docker compose up -d)\n";
        return false;
    }
    const auto info = nlohmann::json::parse(r.text);
    std::cout << "Elasticsearch " << info.at("version").at("number").get<std::string>()
              << " is up.\n";
    return true;
}

void print_stats(const std::vector<geosearch::Poi>& pois) {
    std::map<std::string, std::size_t> by_category;
    std::size_t with_street = 0;
    for (const auto& p : pois) {
        ++by_category[p.category];
        if (p.street) {
            ++with_street;
        }
    }

    std::vector<std::pair<std::string, std::size_t>> sorted(by_category.begin(),
                                                            by_category.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    std::cout << "\nUsable POIs : " << pois.size() << '\n'
              << "With street : " << with_street << '\n'
              << "Categories  : " << sorted.size() << '\n';
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::string data_path = argc > 1 ? argv[1] : "data/brno-poi.json";

    try {
        if (!ping_elasticsearch()) {
            return 1;
        }
        const auto pois = geosearch::load_pois(data_path, "Brno");
        print_stats(pois);

        std::cout << "\nIndexing into 'poi' ...\n";
        const auto stats = geosearch::bulk_index(es_url(), "poi", pois);
        std::cout << "Indexed: " << stats.indexed << ", failed: " << stats.failed << '\n';
        std::cout << "Documents in index: " << geosearch::count_documents(es_url(), "poi")
                  << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}