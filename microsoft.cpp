#include <curl/curl.h>
#include <cstring>
#include "location.cpp"
#include "lib/nrex.cpp"
#include <chrono>
#include <ctime>
#include "utils.cpp"

#pragma warning(disable : 4996)

namespace microsoft_loc {

const char part1[] = "<GetLocationUsingFingerprint xmlns=\"http://inference.location.live.com\"><RequestHeader><Timestamp>";
const char part2[] = "</Timestamp><ApplicationId>e1e71f6b-2149-45f3-a298-a20682ab5017</ApplicationId><TrackingId>21BF9AD6-CFD3-46B3-B041-EE90BD34FDBC</TrackingId><DeviceProfile ClientGuid=\"0fc571be-4624-4ce0-b04e-911bdeb1a222\" Platform=\"Windows7\" DeviceType=\"PC\" OSVersion=\"7600.16695.amd64fre.win7_gdr.101026-1503\" LFVersion=\"9.0.8080.16413\" ExtendedDeviceInfo=\"\" /><Authorization /></RequestHeader><BeaconFingerprint><Detections><Wifi7 BssId=\"";
const char part3[] = "\" rssi=\"-1\" /></Detections></BeaconFingerprint></GetLocationUsingFingerprint>";

char* get_current_time() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    const tm *parts = std::localtime(&now_c);
    char* buffer = new char[30];
    std::strftime(buffer, 20, "%Y-%m-%dT%H:%M:%S", parts);
    return buffer;
}

Location find_location_in_response(char* response) {
    static nrex source_regex("<ResolverStatus Status=\"Success\" Source=\"([^\"]*)\"\\/>");
    nrex_result* source_cap = new nrex_result[source_regex.capture_size()];
    if(!source_regex.match(response, source_cap)) return Location();
    if(source_cap[1].length != 8) return Location();
    if(memcmp(response + source_cap[1].start, "Internal", source_cap[1].length) != 0) return Location();
    delete[] source_cap;
    static nrex location_regex("<ResolvedPosition\\s*Latitude=\"([0-9\\.]+)\"\\s*Longitude=\"([0-9\\.]+)\"\\s*Altitude=\"([0-9\\.]+)\"\\s*\\/>");
    nrex_result* location_cap = new nrex_result[location_regex.capture_size()];
    if (!location_regex.match(response, location_cap)) return Location();
    response[location_cap[1].start + location_cap[1].length] = '\0';
    response[location_cap[2].start + location_cap[2].length] = '\0';
    Location return_location = Location(std::strtof(response + location_cap[1].start, nullptr), std::strtof(response + location_cap[2].start, nullptr), (uint64_t)0);
    delete[] location_cap;
    return return_location;
}

Location get_location(uint64_t bssid) {
    size_t resp_len;
    uint16_t pos = 0;
    char* data = new char[1000];
    memcpy(data, part1, sizeof(part1) - 1);
    pos += sizeof(part1) - 1;
    char* time = get_current_time();
    uint8_t time_len = strlen(time);
    memcpy(data + pos, time, time_len);
    delete[] time;
    pos += time_len;
    memcpy(data + pos, part2, sizeof(part2) - 1);
    pos += sizeof(part2) - 1;
    pos += utils::encode_bssid(data + pos, bssid);
    memcpy(data + pos, part3, sizeof(part3));
    const char* url = "https://inference.location.live.net/inferenceservice/v21/Pox/GetLocationUsingFingerprint";
    uint8_t* response;
    utils::curl_request_post(url, (uint8_t*)data, strlen(data), response, resp_len, {"Content-Type: application/xml"});
    delete[] data;
    Location res = find_location_in_response(reinterpret_cast<char*>(response));
    res.raw_response = response;
	res.raw_response_len = resp_len;
    return res;
}
}
