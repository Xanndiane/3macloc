#define TMACLOC_VERSION "0.1"

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4996)

#include <iostream>
#include <cstdint>
#include "google.cpp"
#include "yandex.cpp"
#include "apple.cpp"
#include "microsoft.cpp"
#include "mylnikov.cpp"
#include <iomanip>
#include "results_share.cpp"
#include "utils.cpp"

#define ISHEXCHR(s) ((s >= '0' && s <= '9') || (s >= 'a' && s <= 'f') || (s >= 'A' && s <= 'F'))
#define ISDOTCHR(s) (s == ':' || s == '-' || s == '_')

enum class bssidtype {
	Invalid,
	Valid,
	Valid_with_dots
};

bssidtype check_bssid(const char* bssid) {
    uint32_t bssid_len = strlen(bssid);
    if(bssid_len == 17) {
        for(int i = 0; i < bssid_len; i++) {
            bool char_valid = false;
            if((i + 1) % 3 == 0)
                char_valid = ISDOTCHR(bssid[i]);
            else
                char_valid = ISHEXCHR(bssid[i]);
            if(!char_valid) return bssidtype::Invalid;
        }
		return bssidtype::Valid_with_dots;
    } else if(bssid_len == 12) {
        for(int i = 0; i < bssid_len; i++) {
            if(!ISHEXCHR(bssid[i])) return bssidtype::Invalid;
        }
    } else {
        return bssidtype::Invalid;
    }
    return bssidtype::Valid;
}

bool bssid_to_int(char* sbssid, uint64_t &bssid) {
	switch(check_bssid(sbssid)) {
		case bssidtype::Invalid:
			return false;
	    case bssidtype::Valid:
		    break;
        case bssidtype::Valid_with_dots:
		    delete_2dots(sbssid);
            break;
	};
	bssid = std::strtoull(sbssid, nullptr, 16);
	return true;
}

bool get_bssid_from_user(uint64_t &bssid) {
	char* sbssid = new char[100];
	std::cin >> std::setw(50) >> sbssid;
	bool ret = bssid_to_int(sbssid, bssid);
	delete[] sbssid;
	return ret;
}

inline void print_help() {
	std::cout << 
R"(3macloc v1 
Open source WiFi BSSID locator.

Usage:
  3macloc [options]

Options:
  --help, -h        Print help message.
  -b <string>       BSSID to search.
  --precision <int> Output float precision.
  --json            Print result in JSON format.
  -DM               Disable Microsoft source.
  -DI               Disable Myilnikov source.
  -DA               Disable Apple source.
  -DG               Disable Google source.
  -DY               Disable Yandex source.
  -DW               Disable 3WiFI source.
  --singlethread    Disable concurrent requests.
)";
}

struct Settings {
	bool disable_microsoft = false;
	bool disable_google = false;
	bool disable_yandex = false;
	bool disable_3wifi = false;
	bool disable_apple = false;
	bool disable_myilnikov = false;
	bool print_help = false;
	bool print_to_json = false;
	bool disable_multithread = false;
	bool bssid_in_args = false;
	uint64_t bssid;
	int precision = 10;
};

#define CMPARG(x) strcmp(argv[i], x) == 0

inline Settings get_settings(int argc, char** argv) {
	Settings settings;
	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-b") == 0)  {
			if(i + 1 >= argc) { // if no int in args. Example: 3macloc -b
				std::cout << "Invalid argument: -b <int> will not receive <int>\n";
			} else { // if int in args. Example: 3macloc -b 000000000000
				settings.bssid_in_args = bssid_to_int(argv[i + 1], settings.bssid);
				if(!settings.bssid_in_args) {
					std::cout << "You passed the wrong BSSID to the -b argument\n";
				}
				i++; // skip <int>
			}
		}
		else if(strcmp(argv[i], "--json") == 0) settings.print_to_json = true;
		else if(strcmp(argv[i], "--precision") == 0) {
			if(i + 1 >= argc) { // if no int in args. Example: 3macloc --precision
				std::cout << "Invalid argument: --precision <int> will not receive <int>\n";
			} else { // if int in args. Example: 3macloc --precision 20
				settings.precision = atoi(argv[i + 1]);
				i++; // skip <int>
			}
		}
		else if(CMPARG("-DM")) settings.disable_microsoft = true;
		else if(CMPARG("-DG")) settings.disable_google = true;
		else if(CMPARG("-DY")) settings.disable_yandex = true;
		else if(CMPARG("-DW")) settings.disable_3wifi = true;
		else if(CMPARG("-DA")) settings.disable_apple = true;
		else if(CMPARG("-DI")) settings.disable_myilnikov = true;
		else if(CMPARG("--help") or CMPARG("-h")) settings.print_help = true;
		else if(CMPARG("--singlethread")) settings.disable_multithread = true;
		else std::cout << "Invalid argument: " << argv[i] << '\n';
	}
	return settings;
}

int main(int argc, char** argv) {
	Settings settings = get_settings(argc, argv);
	if(settings.print_help) {
		print_help();
		return 0;
	}
	std::cout.precision(settings.precision);
	bool valid_bssid = settings.bssid_in_args;
	uint64_t bssid = settings.bssid;
	if(!valid_bssid) {
		do {
			std::cout << "Enter BSSID: ";
			valid_bssid = get_bssid_from_user(bssid);
			if(!valid_bssid) {
				std::cout << "Invalid BSSID\n";
			}
		} while(!valid_bssid);
	}
	std::vector<std::pair<const char*, Location>> results;
	if(!settings.disable_google) {
		Location google_loc = google_loc::get_location(bssid);
		results.push_back({"google", google_loc});
		if(!settings.print_to_json) {
			std::cout << "Google: " << google_loc.lat << " " << google_loc.lon << std::endl;
		}
	}
	if(!settings.disable_yandex) {
		Location yandex_loc = yandex_loc::get_location(bssid);
		results.push_back({"yandex", yandex_loc});
		if(!settings.print_to_json) {
			std::cout << "Yandex: " << yandex_loc.lat << " " << yandex_loc.lon << std::endl; 
		}
	}
	if(!settings.disable_apple) {
		Location apple_loc = apple_loc::get_location(bssid);
		results.push_back({"apple", apple_loc});
		if(!settings.print_to_json) {
			std::cout << "Apple: " << apple_loc.lat << " " << apple_loc.lon << std::endl;
		}
	}
	if(!settings.disable_microsoft) {
		Location microsoft_loc = microsoft_loc::get_location(bssid);
		results.push_back({"microsoft", microsoft_loc});
		if(!settings.print_to_json) {
			std::cout << "Microsoft: " << microsoft_loc.lat << " " << microsoft_loc.lon << std::endl;
		}
	}
	if(!settings.disable_myilnikov) {
		Location mylnikov_loc = mylnikov_loc::get_location(bssid);
		results.push_back({"mylnikov", mylnikov_loc});
		if(!settings.print_to_json) {
			std::cout << "Mylnikov: " << mylnikov_loc.lat << " " << mylnikov_loc.lon << std::endl;
		}
	}
	if(settings.print_to_json) {
		std::cout << utils::construct_json_output(results, settings.precision);
	}
	//load_results_to_server(results, bssid);
	return 0;
}
