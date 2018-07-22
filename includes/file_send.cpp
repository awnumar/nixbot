#include "file_send.h"
#include <fstream>
#include <unistd.h>
#include <arpa/inet.h>

file_send::file_send(std::string file_path, long file_size, int **sock) {
	this->file_path = file_path;
	this->file_size = file_size;
	this->sock = sock;
}

file_send::~file_send() {
}

void file_send::upload_file() {
	long current_size = 0;
	char starts_with[] = "nixbot_file_bytes::";

	std::ifstream input(file_path.c_str(), std::ios::binary);

	int times = (file_size / 4075) + 1;
	for (int i = 0; i < times; i++) {
		int to_allocate = file_size - current_size;
		if (to_allocate > 4075) {
			to_allocate = 4075;
			current_size += to_allocate;
		}

		char *buffer = new char[to_allocate + 20];
		memcpy(buffer, &starts_with, 19);
		input.read(&buffer[19], to_allocate);

        buffer[to_allocate + 19] = '\0'; //add the \0 since our server message delimiter is \0nixbot_
		if (send_data(sock, buffer, to_allocate + 20) == 0) {
			delete[] buffer;
			goto delete_class;
		}

		delete[] buffer;
	}

	input.close();

	std::cout << "[INFO] Finished sending" << std::endl;

    delete_class:
    delete file_send_class;
}

int file_send::send_data(int **sock, char *data, int size) {
    int bytecount = send(**sock, data, size, 0);
    if(bytecount == -1) {
        std::cout << "Error sending data: " << errno << std::endl;
        shutdown(**sock, 2);
        close(**sock);
        delete *sock;
        *sock = nullptr;
        return 0;
    }

    return bytecount;
}
