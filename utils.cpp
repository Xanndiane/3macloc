#pragma once

#include "location.cpp"
#include "lib/base64.c"
#include <cstring>
#include <curl/curl.h>
#include <zlib.h>
#include <vector>
#include <string>

namespace utils {

char hex_chars[] = "0123456789ABCDEF";

template <typename T>
size_t encode_bssid(T* buffer, uint64_t bssid, bool with_dots=true) {
    uint8_t pos = 0;
    for(uint8_t i = 0; i < 6; i++) {
        buffer[pos++] = utils::hex_chars[(bssid >> (8 * (5 - i) + 4)) & 0xF];
        buffer[pos++] = utils::hex_chars[(bssid >> (8 * (5 - i))) & 0xF];
        if(i != 5 && with_dots) {
            buffer[pos++] = ':';
        }
    }
	buffer[pos] = '\0';
    return pos;
}

void location_to_rawb64(Location loc, std::string &server_out, const char* server_name) {
	char* rawb64_json_obj = new char[loc.raw_response_len * 2 + 100];
	rawb64_json_obj[0] = '"';
	strcpy(rawb64_json_obj + 1, server_name);
	strcat(rawb64_json_obj, "\":{\"rawb64\":\"");
	b64_encode(loc.raw_response, loc.raw_response_len, reinterpret_cast<unsigned char*>(rawb64_json_obj + strlen(rawb64_json_obj)));
	strcat(rawb64_json_obj, "\"},");
	server_out.append(rawb64_json_obj);
	delete[] rawb64_json_obj;
}

uint8_t* zlib_compress(uint8_t* data, size_t dsize, size_t& osize) {
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        throw(std::runtime_error("deflateInit2 failed while compressing."));
    }
    zs.next_in = reinterpret_cast<Bytef*>(data);
    zs.avail_in = static_cast<uInt>(dsize); // set the input data
    int ret;
    uint8_t* out = new uint8_t[10240];
    uint8_t* buffer = new uint8_t[1024];
    osize = 0;
    // retrieve the compressed bytes blockwise
    do {
        zs.next_out = reinterpret_cast<Bytef*>(buffer);
        zs.avail_out = 1024;
        ret = deflate(&zs, Z_FINISH);
        memcpy(out + osize, buffer, zs.total_out);
        osize += zs.total_out;
    } while (ret == Z_OK);
    deflateEnd(&zs);
    if (ret != Z_STREAM_END) { // an error occurred that was not EOF
        throw(std::runtime_error(std::string("Exception during zlib compression: ") + zs.msg));
    }
    delete[] buffer;
    return out;
}

uint8_t* zlib_decompress(uint8_t* compressedData, size_t len, size_t& out_len) {
    out_len = 0;
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = static_cast<uInt>(len);
    stream.next_in = reinterpret_cast<Bytef*>(compressedData);

    if (inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK)
        return nullptr;
	
    int ret;
    uint8_t* decompressedData = new uint8_t[len * 5];
    const int BUFFER_SIZE = 10240;
    char* buffer = new char[BUFFER_SIZE];
    do {
        stream.avail_out = BUFFER_SIZE;
        stream.next_out = reinterpret_cast<Bytef*>(buffer);
        ret = inflate(&stream, Z_NO_FLUSH);
        if (ret == Z_NEED_DICT or ret < 0) {
            out_len = 0;
            delete[] decompressedData;
			delete[] buffer;
            return nullptr;
        }
        int have = BUFFER_SIZE - stream.avail_out;
        memcpy(decompressedData + out_len, buffer, have);
        out_len += have;
    } while (ret != Z_STREAM_END);
	
    delete[] buffer;
	
    if (inflateEnd(&stream) != Z_OK) {
		out_len = 0;
        delete[] decompressedData;
        return nullptr;
    }

    return decompressedData;
}

#define CURLBUFFERSIZE 10240

size_t curlWRITEDATA(const char* ptr, const size_t size, const size_t nmemb, void* stream) {
    size_t realsize = size * nmemb;
    size_t* cur_len = reinterpret_cast<size_t*>(reinterpret_cast<uint8_t*>(stream) + (CURLBUFFERSIZE - sizeof(size_t)));
    if(*cur_len + realsize + 1 > CURLBUFFERSIZE) {
		throw std::overflow_error("utils.cpp:curlWRITEDATA buffer overflow error");
	}
	memcpy(reinterpret_cast<char*>(stream) + *cur_len, ptr, realsize);
    *cur_len += realsize;
    return realsize;
}

bool curl_request_get(const char* request_url, uint8_t*& response_data, size_t& response_size, std::vector<const char*> headers = {}) {
    CURL* curl = curl_easy_init();
    response_data = new uint8_t[CURLBUFFERSIZE];
    *reinterpret_cast<size_t*>(response_data + (CURLBUFFERSIZE - sizeof(size_t))) = 0;
    if (curl) {
        curl_slist* curl_headers = NULL;
        if(headers.size() > 0) {
            for(const char* header : headers) {
                curl_headers = curl_slist_append(curl_headers, header);
            }
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
        }
        curl_easy_setopt(curl, CURLOPT_URL, request_url);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWRITEDATA);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void*>(response_data));
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if(headers.size() > 0) {
            curl_slist_free_all(curl_headers);
        }
    } else {
        return false;
    }
    response_size = *reinterpret_cast<size_t*>(response_data + (CURLBUFFERSIZE - sizeof(size_t)));
    return true;
}

bool curl_request_post(const char* request_url, uint8_t* request_data, size_t request_data_length, uint8_t*& response_data, size_t& response_size, std::vector<const char*> headers = {}) {
    CURL* curl = curl_easy_init();
    response_data = new uint8_t[CURLBUFFERSIZE];
    *reinterpret_cast<size_t*>(response_data + (CURLBUFFERSIZE - sizeof(size_t))) = 0;
    if (curl) {
        curl_slist* curl_headers = NULL;
        if(headers.size() > 0) {
            for(const char* header : headers) {
                curl_headers = curl_slist_append(curl_headers, header);
            }
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
        }
        curl_easy_setopt(curl, CURLOPT_URL, request_url);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request_data_length);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWRITEDATA);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void*>(response_data));
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if(headers.size() > 0) {
            curl_slist_free_all(curl_headers);
        }
    } else {
        return false;
    }
    response_size = *reinterpret_cast<size_t*>(response_data + (CURLBUFFERSIZE - sizeof(size_t)));
    return true;
}

std::string construct_json_output(std::vector<std::pair<const char*, Location>> results, int precision=10) {
    std::string json_output("{\"ok\":true,\"results\":{");
    for(auto result : results) {
        json_output.append("\"");
        json_output.append(result.first);
        json_output.append("\":");
        json_output.append(result.second.to_json(precision));
        json_output.append(",");
    }
	if(json_output.back() == ',') json_output.pop_back();
	json_output.append("}}");
	return json_output;
}

}
