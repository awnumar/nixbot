#ifndef SSH_BRUTEFORCE_H
#define SSH_BRUTEFORCE_H

#include <iostream>
#include <string>
#include <vector>
#include <libssh2.h>

class ssh_bruteforce
{
    public:
        ssh_bruteforce(char *[]);
        virtual ~ssh_bruteforce();

        std::vector<std::string> passwords;
        std::string bruteforce();
        void clean(LIBSSH2_SESSION **, int *);
    private:
        unsigned long hostaddr;
        std::string port;
        std::string username;
};

#endif // SSH_BRUTEFORCE_H
