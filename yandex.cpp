#include <curl/curl.h>
#include <cstring>
#include "lib/nrex.cpp"
#include "location.cpp"
#include "utils.cpp"

#pragma warning(disable : 4996)

namespace yandex_loc {

Location find_location_in_response(char* response) {
    static nrex source_regex;
    source_regex.compile("<location source=\"([^\"]*)\">");
    nrex_result* source_cap = new nrex_result[source_regex.capture_size()];
    if(!source_regex.match(response, source_cap)) return Location();
    if(source_cap[1].length != 11) return Location();
    if(memcmp(response + source_cap[1].start, "FoundByWifi", source_cap[1].length) != 0) return Location();
    delete[] source_cap;
    static nrex coords_regex;
    coords_regex.compile("<coordinates\\s*latitude=\"([0-9\\.]+)\"\\s*longitude=\"([0-9\\.]+)\" nlatitude=\"([0-9\\.]+)\"\\s*nlongitude=\"([0-9\\.]+)\"\\s*\\/>");
    nrex_result* coords_cap = new nrex_result[coords_regex.capture_size()];
    if (!coords_regex.match(response, coords_cap)) return Location();
    response[coords_cap[1].start + coords_cap[1].length] = '\0';
    response[coords_cap[2].start + coords_cap[2].length] = '\0';
    auto ret = Location(std::strtof(response + coords_cap[1].start, nullptr), std::strtof(response + coords_cap[2].start, nullptr), (uint64_t)0);
	response[coords_cap[1].start + coords_cap[1].length] = '"';
    response[coords_cap[2].start + coords_cap[2].length] = '"';
    delete[] coords_cap;
	return ret;
}

Location get_location(uint64_t bssid) {
    size_t resp_len;
    char* url = new char[200];
    strcpy(url, "https://api.lbs.yandex.net/cellid_location?wifinetworks="); //56
    utils::encode_bssid(url + strlen(url), bssid, false);
    strcat(url, "%3A-65");
    uint8_t* response_data;
    utils::curl_request_get(url, response_data, resp_len);
    response_data[resp_len] = 0;
    Location res = find_location_in_response(reinterpret_cast<char*>(response_data));
	res.raw_response = response_data;
	res.raw_response_len = resp_len;
	return res;
}

}
