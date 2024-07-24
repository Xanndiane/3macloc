#include <string>
#include <cstring>
#include <curl/curl.h>
#include "location.cpp"
#include <cstdint>
#include "utils.cpp"

namespace google_loc {

const uint8_t data1_1[] = {
    0x0A, 0x66, 0x0A, 0x04, 0x32, 0x30, 0x32, 0x31, 0x12, 0x57, 0x61, 0x6E, 0x64, 0x72, 0x6F, 0x69,
    0x64, 0x2F, 0x4C, 0x45, 0x41, 0x47, 0x4F, 0x4F, 0x2F, 0x66, 0x75, 0x6C, 0x6C, 0x5F, 0x77, 0x66,
    0x35, 0x36, 0x32, 0x67, 0x5F, 0x6C, 0x65, 0x61, 0x67, 0x6F, 0x6F, 0x2F, 0x77, 0x66, 0x35, 0x36,
    0x32, 0x67, 0x5F, 0x6C, 0x65, 0x61, 0x67, 0x6F, 0x6F, 0x3A, 0x36, 0x2E, 0x30, 0x2F, 0x4D, 0x52,
    0x41, 0x35, 0x38, 0x4B, 0x2F, 0x31, 0x35, 0x31, 0x31, 0x31, 0x36, 0x31, 0x37, 0x37, 0x30, 0x3A,
    0x75, 0x73, 0x65, 0x72, 0x2F, 0x72, 0x65, 0x6C, 0x65, 0x61, 0x73, 0x65, 0x2D, 0x6B, 0x65, 0x79,
    0x73, 0x2A, 0x05, 0x65, 0x6E, 0x5F, 0x55, 0x53, 0x22, 0x22, 0x12, 0x1E, 0x08, 0xA3, 0xF7, 0x09,
    0x12, 0x0A, 0x0A, 0x00, 0x40
};

const uint8_t data1_3[] = {
    0x12, 0x0A, 0x0A, 0x00, 0x40, 0xE6, 0xAA, 0x91, 0x9A, 0xA3, 0xA4, 0x04, 0x18, 0x02, 0x50, 0x00
};

const uint8_t data2_1[] = {
    0x00, 0x02, 0x00, 0x00, 0x1F, 0x6C, 0x6F, 0x63, 0x61, 0x74, 0x69, 0x6F, 0x6E, 0x2C, 0x32, 0x30,
    0x32, 0x31, 0x2C, 0x61, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x2C, 0x67, 0x6D, 0x73, 0x2C, 0x65,
    0x6E, 0x5F, 0x55, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x67, 0x00,
    0x00, 0x00, 0xBB, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x08, 0x67, 0x3A, 0x6C, 0x6F, 0x63, 0x2F,
    0x71, 0x6C, 0x00, 0x00, 0x00, 0x04, 0x50, 0x4F, 0x53, 0x54, 0x6D, 0x72, 0x00, 0x00, 0x00, 0x04,
    0x52, 0x4F, 0x4F, 0x54, 0x00, 0x00, 0x00, 0x00
};


uint8_t* encode_varint(uint64_t input) {
    uint8_t* buf = new uint8_t[15];
    uint8_t len = 1;
    while (true) {
        uint8_t towrite = input & 0x7F;
        input >>= 7;
        if (input) {
            buf[len++] = towrite | 0x80;
        }
        else {
            buf[len++] = towrite;
            break;
        }
    }
    buf[0] = len - 1;
    return buf;
}

#define CURLDATASIZE 10240

size_t curlWRITEDATA(const char* ptr, const size_t size, const size_t nmemb, void* stream) {
    size_t realsize = size * nmemb;
	size_t* cur_len = reinterpret_cast<size_t*>(reinterpret_cast<uint8_t*>(stream) + (CURLDATASIZE - sizeof(size_t)));
    if(*cur_len + realsize + 1 > CURLDATASIZE) {
		throw std::overflow_error("google.cpp:curlWRITEDATA buffer overflow error");
	}
	memcpy(reinterpret_cast<char*>(stream) + *cur_len, ptr, realsize);
    *cur_len += realsize;
    return realsize;
}

uint8_t* makeRequest(const uint8_t* data, size_t len, size_t& olen) {
    CURL* curl = curl_easy_init();
    uint8_t* responseData = new uint8_t[CURLDATASIZE];
    *reinterpret_cast<size_t*>(responseData + (CURLDATASIZE - sizeof(size_t))) = 0;
    if (curl) {
        curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/binary");
        headers = curl_slist_append(headers, "User-Agent: GoogleMobile/1.0 (M5 MRA58K); gzip");
        headers = curl_slist_append(headers, "Accept-Encoding: gzip");
        curl_easy_setopt(curl, CURLOPT_URL, "https://www.google.com/loc/m/api");
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, len);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWRITEDATA);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void*>(responseData));
        curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    olen = *reinterpret_cast<size_t*>(responseData + (CURLDATASIZE - sizeof(size_t)));
    return responseData;
}

uint8_t* constructData1(uint64_t bssid, size_t& out_len) {
    uint8_t* data1 = new uint8_t[256];
    memcpy(data1, data1_1, sizeof(data1_1));
    uint8_t* data1_bssid = encode_varint(bssid);
    uint8_t blen = data1_bssid[0];
    memcpy(data1 + 117, data1_bssid + 1, blen);
    delete[] data1_bssid;
    memcpy(data1 + blen + 117, data1_3, sizeof(data1_3));
    out_len = blen + sizeof(data1_1) + sizeof(data1_3);
    return data1;
}

uint8_t* constructData2(const uint8_t* data1, size_t len, size_t& out_len) {
    uint8_t* buf = new uint8_t[1024];
    memcpy(buf, data2_1, sizeof(data2_1)); //88
    buf[88] = len & 0xFF;
    buf[89] = (len >> 8) & 0xFF;
    buf[90] = 0x01;
    buf[91] = 0x67;
    memcpy(buf + 92, data1, len);
    buf[92 + len] = 0x00;
    buf[93 + len] = 0x00;
    out_len = sizeof(data2_1) + 6 + len;
    return buf;
}

Location find_location_in_response(uint8_t* response, size_t resp_len) {
    size_t pos_gzip = resp_len + 1;
    for (int i = 0; i < ((int)resp_len - (int)3); i++) {
        if (response[i] == 0x1F && response[i + 1] == 0x8B && response[i + 2] == 0x08) {
            pos_gzip = i;
            break;
        }
    }
    if (pos_gzip != resp_len + 1) {
        size_t decm_len;
        uint8_t* decompressedData = utils::zlib_decompress(response + pos_gzip, (int)resp_len - pos_gzip, decm_len);
        if (decm_len == 0) {
            return Location();
        }
        for (size_t i = 0; i + 12 < decm_len; i++) {
            if (decompressedData[i] == 0x0A && decompressedData[i + 1] == 0x0A && decompressedData[i + 12] == 24) {
                int lat = decompressedData[i + 3];
                lat |= decompressedData[i + 4] << 8;
                lat |= decompressedData[i + 5] << 16;
                lat |= decompressedData[i + 6] << 24;
                int lon = decompressedData[i + 8];
                lon |= decompressedData[i + 9] << 8;
                lon |= decompressedData[i + 10] << 16;
                lon |= decompressedData[i + 11] << 24;
                delete[] decompressedData;
                return Location(lat / 10000000.0, lon / 10000000.0);
            }
        }
    }
    return Location();
}

Location get_location(uint64_t bssid) {
    size_t raw_data1_len;
    uint8_t* raw_data1 = constructData1(bssid, raw_data1_len);
    size_t comp_data1_len;
    uint8_t* comp_data1 = utils::zlib_compress(raw_data1, raw_data1_len, comp_data1_len);
    delete[] raw_data1;
    size_t data2_len;
    uint8_t* data2 = constructData2(comp_data1, comp_data1_len, data2_len);
    delete[] comp_data1;
    size_t resp_len;
    uint8_t* response = makeRequest(data2, data2_len, resp_len);
    delete[] data2;
    Location result = find_location_in_response(response, resp_len);
    result.raw_response = response;
	result.raw_response_len = resp_len;
    return result;
}

}
