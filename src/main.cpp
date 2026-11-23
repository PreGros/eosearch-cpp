#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

std::string es_url() {
    if (const char* v = std::getenv("ES_URL")) {
        return v;
    }
    return "http://localhost:9200";
}

}  // namespace

int main() {
    const std::string url = es_url();
    std::cout << "Connecting to Elasticsearch at " << url << " ...\n";

    const cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Timeout{5000});

    if (r.error) {
        std::cerr << "Connection failed: " << r.error.message << '\n'
                  << "Is the container running? Try: docker compose up -d\n";
        return 1;
    }
    if (r.status_code != 200) {
        std::cerr << "Unexpected HTTP status " << r.status_code << ": " << r.text << '\n';
        return 1;
    }

    try {
        const auto info = nlohmann::json::parse(r.text);
        std::cout << "Cluster name : " << info.at("cluster_name").get<std::string>() << '\n'
                  << "ES version   : " << info.at("version").at("number").get<std::string>() << '\n'
                  << "Tagline      : " << info.value("tagline", "") << '\n';
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Could not parse Elasticsearch response: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
