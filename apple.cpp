#include <string>
#include <cstring>
#include <iostream>
#include <curl/curl.h>
#include "location.cpp"
#include "lib/picoproto.cc"
#include "utils.cpp"
#include <vector>

namespace apple_loc {

const uint8_t data1_1[] = {
    0x00, 0x01, 0x00, 0x05, 0x65, 0x6E, 0x5F, 0x55, 0x53, 0x00,
    0x13, 0x63, 0x6f, 0x6d, 0x2e, 0x61, 0x70, 0x70, 0x6c, 0x65,
    0x2e, 0x6c, 0x6f, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x64,
    0x00, 0x0A, 0x38, 0x2e, 0x31, 0x2e, 0x31, 0x32, 0x42, 0x34,
    0x31, 0x31, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x19,
    0x12, 0x13, 0x0a, 0x11
};

const uint8_t data1_3[] = {
    0x18, 0x00, 0x20, 0x01
};

uint8_t* constructRequest(uint64_t bssid, size_t& out_len) {
    uint8_t* request_data = new uint8_t[sizeof(data1_1) + sizeof(data1_3) + 100];
    memcpy(request_data, data1_1, sizeof(data1_1));
    size_t bssid_len = utils::encode_bssid(request_data + sizeof(data1_1), bssid);
    memcpy(request_data + sizeof(data1_1) + bssid_len, data1_3, sizeof(data1_3));
    out_len = bssid_len + sizeof(data1_1) + sizeof(data1_3);
    return request_data;
}

uint64_t from_apple_bssid_format(const char* bssid) {
	char* out = new char[20];
	int out_len = 0;
	int bssid_len = strlen(bssid);
	int last_dots = -1;
	for(int i = 0; i <= bssid_len; i++) {
		if(bssid[i] == ':' || i == bssid_len) {
			int num_hex = i - last_dots - 1;
			switch(num_hex) {
				case 0:
					out[out_len++] = '0';
					out[out_len++] = '0';
					break;
				case 1:
					out[out_len++] = '0';
					out[out_len++] = bssid[i - 1];
					break;
				case 2:
					out[out_len++] = bssid[i - 2];
					out[out_len++] = bssid[i - 1];
					break;
			}
			last_dots = i;
		}
	}
	out[out_len] = '\0';
	uint64_t ret = std::strtoull(out, nullptr, 16);
	delete[] out;
	return ret;
}

#define NETWORK404VAL (uint64_t)18446744055709551616U 

Location find_location_in_response(uint8_t* response, size_t resp_len, uint64_t source_bssid) {
	picoproto::Message message;
	message.ParseFromBytes(response + 10, resp_len - 10);
	std::vector<picoproto::Message*> networks = message.GetMessageArray(2);
	for(auto network : networks) {
		uint64_t bssid = from_apple_bssid_format(network->GetString(1).c_str());
		if(source_bssid == bssid) {
			picoproto::Message* network_location_data = network->GetMessage(2);
			uint64_t lat = network_location_data->GetUInt64(1);
			uint64_t lon = network_location_data->GetUInt64(2);
			if(lat == NETWORK404VAL || lon == NETWORK404VAL) { // Network not found
				return Location();
			}
			return Location(lat / 100000000.0, lon / 100000000.0);
		}
	}
	return Location();
}

Location get_location(uint64_t bssid) {
    size_t request_len;
    uint8_t* request_data = constructRequest(bssid, request_len);
    size_t response_len;
    uint8_t* response;
    std::vector<const char*> headers = {
        "Content-Type: application/x-www-form-urlencoded",
        "Accept: */*",
        "Accept-Charset: utf-8",
        "Accept-Language: en-us",
        "User-Agent: locationd/1753.17 CFNetwork/711.1.12 Darwin/14.0.0"
    };
    utils::curl_request_post("https://gs-loc.apple.com/clls/wloc", request_data, request_len, response, response_len, headers);
    delete[] request_data;
	Location result = find_location_in_response(response, response_len, bssid);
	result.raw_response = response;
	result.raw_response_len = response_len;
    return result;
}
}
