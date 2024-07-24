#pragma once

#include <sstream>
#include <cstdint>
#include <cstring>

void delete_2dots(char* in) {
    in[2] = in[3];
    in[3] = in[4];
    in[4] = in[6];
    in[5] = in[7];
    in[6] = in[9];
    in[7] = in[10];
    in[8] = in[12];
    in[9] = in[13];
    in[10] = in[15];
    in[11] = in[16];
    in[12] = '\0';
}

struct Location {
    float lat = 0;
    float lon = 0;
    uint64_t sourceBssid = 0;
    bool valid = false;
	uint8_t* raw_response;
	size_t raw_response_len = 0;
    Location() {}
    Location(float _lat, float _lon) {
        lat = _lat;
        lon = _lon;
        valid = true;
    }
    Location(float _lat, float _lon, uint64_t _sourceBssid) {
        lat = _lat;
        lon = _lon;
        sourceBssid = _sourceBssid;
        valid = true;
    }
    Location(float _lat, float _lon, const char* _sourceBssid) {
        lat = _lat;
        lon = _lon;
		size_t bssidLen = strlen(_sourceBssid);
		if(bssidLen == 17) {
			char* bssidBuf = new char[bssidLen + 1];
			memcpy(bssidBuf, _sourceBssid, bssidLen + 1);
			delete_2dots(bssidBuf);
			sourceBssid = std::strtoull(bssidBuf, nullptr, 16);
			delete[] bssidBuf;
		} else {
			sourceBssid = std::strtoull(_sourceBssid, nullptr, 16);
		}
		valid = true;
    }
	std::string to_json(int precision=10) {
		std::ostringstream out;
		out << "{\"found\":" << (valid ? "true" : "false");
		if(valid) {
			out.precision(precision);
			out << ",\"lat\":" << lat << ",\"lon\":" << lon;
		}
		out << "}";
		return out.str();
	}
};
