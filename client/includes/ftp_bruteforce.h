#ifndef FTP_BRUTEFORCE_H
#define FTP_BRUTEFORCE_H

#include <iostream>
#include <string>
#include <vector>

class ftp_bruteforce
{
    public:
        ftp_bruteforce(char *[]);
        virtual ~ftp_bruteforce();

        std::vector<std::string> passwords;
        std::string bruteforce();
    private:
        std::string host;
        std::string username;
};

#endif // FTP_BRUTEFORCE_H
