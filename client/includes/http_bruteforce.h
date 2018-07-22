#ifndef HTTP_BRUTEFORCE_H
#define HTTP_BRUTEFORCE_H

#include <iostream>
#include <string>
#include <vector>

class http_bruteforce
{
    public:
        http_bruteforce(char *[]);
        virtual ~http_bruteforce();

        std::vector<std::string> passwords;
        std::string bruteforce();
    private:
        std::string link;
        std::string post_data;
        std::string user_agent;
        char follow_redirections = '0';
        std::string valid_string;
};

#endif // HTTP_BRUTEFORCE_H
