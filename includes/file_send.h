#ifndef FILE_SEND_H
#define FILE_SEND_H

#include <iostream>
#include <string.h>
#include <vector>

class file_send
{
    public:
        file_send(std::string, long, int **);
        virtual ~file_send();
		file_send *file_send_class = nullptr;
		void upload_file();
	private:
		std::string file_path;
		long file_size;
		int **sock;

		int send_data(int **, char *, int);
};

#endif // FILE_SEND_H
