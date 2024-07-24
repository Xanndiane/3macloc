#include <curl/curl.h>
#include <cstring>
#include "lib/nrex.cpp"
#include "location.cpp"
#include "utils.cpp"

namespace mylnikov_loc {

bool nrex_result_cmp(nrex_result res, const char* str, const char* initial_str) {
	size_t _strlen = strlen(str);
	if(res.length != _strlen) return false;
	if(memcmp(initial_str + res.start, str, _strlen) != 0) return false;
	return true;
}

inline int nrex_result_to_int(nrex_result res, char* initial_str) {
	size_t strend = res.start + res.length;
	char b = initial_str[strend];
	initial_str[strend] = '\0';
	int ret = atoi(initial_str + res.start);
	initial_str[strend] = b;
	return ret;
}

inline float nrex_result_to_float(nrex_result res, char* initial_str) {
	size_t strend = res.start + res.length;
	char b = initial_str[strend];
	initial_str[strend] = '\0';
	float ret = atof(initial_str + res.start);
	initial_str[strend] = b;
	return ret;
}

Location find_location_in_response(char* response) {
    static nrex json_field_regex;
    json_field_regex.compile("\"([^\"]*)\"\\s*:\\s*([0-9\\.]+)\\s*");
    nrex_result* json_field_cap = new nrex_result[json_field_regex.capture_size()];
	int offset = 0;
	bool result_field = false, lat_field = false, lon_field = false;
	float lat, lon;
	size_t response_strlen = strlen(response);
	while(true) {
		if(!json_field_regex.match(response + offset, json_field_cap)) {
			break;
		}
		if(nrex_result_cmp(json_field_cap[1], "result", response + offset)) {
			result_field = true;
			int result_code = nrex_result_to_int(json_field_cap[2], response + offset);
			if(result_code != 200) { return Location(); }
		} else if(nrex_result_cmp(json_field_cap[1], "lat", response + offset)) {
			lat_field = true;
			lat = nrex_result_to_float(json_field_cap[2], response + offset);
		} else if(nrex_result_cmp(json_field_cap[1], "lon", response + offset)) {
			lon_field = true;
			lon = nrex_result_to_float(json_field_cap[2], response + offset);
		}

		offset += json_field_cap[0].length;
		if(offset >= response_strlen) break;
	}
	delete[] json_field_cap;
	if(result_field && lat_field && lon_field) {
		return Location(lat, lon, (uint64_t)0);
	} else {
		return Location();
	}
}

Location get_location(uint64_t bssid, bool p3wifi_url=false) {
    size_t resp_len;
    char* url = new char[200];
    if(p3wifi_url) {
		strcpy(url, "https://3wifi.dev/3macloc?v=1&bssid=");
	} else {
		strcpy(url, "https://api.mylnikov.org/geolocation/wifi?v=1.1&bssid=");
	}
    utils::encode_bssid(url + strlen(url), bssid, false);
    uint8_t* response_data;
    utils::curl_request_get(url, response_data, resp_len);
    response_data[resp_len] = 0;
	Location res = find_location_in_response(reinterpret_cast<char*>(response_data));
	res.raw_response = response_data;
	res.raw_response_len = resp_len;
	return res;
}

}
