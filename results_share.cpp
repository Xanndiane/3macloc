#include <curl/curl.h>
#include <vector>
#include "location.cpp"
#include "utils.cpp"
#include <cstring>
#include <iostream>
#include <cstdint>

#define SERVERURL "http://127.0.0.1:8080/3macloc"
#define USER_AGENT {"User-Agent: 3macloc_0.1"}

void load_results_to_server(std::vector<std::pair<const char*, Location>> results, uint64_t bssid) {
    std::string to_server_output = "{\"bssid\":";
	to_server_output.append(std::to_string(bssid));
    to_server_output.append(",\"results\":{");
    for (size_t i = 0; i < results.size(); i++) {
        utils::location_to_rawb64(results[i].second, to_server_output, results[i].first);
    }
    if(to_server_output.back() == ',') to_server_output.pop_back();
	to_server_output.append("}}");
    std::cout << to_server_output << "\n\n";
    uint8_t* response_data;
    size_t response_length;
    utils::curl_request_post(SERVERURL, (uint8_t*)to_server_output.c_str(), to_server_output.length(), response_data, response_length, USER_AGENT);
    response_data[response_length] = 0;
    std::cout << (char*)response_data << '\n';
}
