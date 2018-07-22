#include "execute.h"

#include <unistd.h>
#include <errno.h>

execute::execute(std::string command, std::string *output, unsigned short *function_return) {
    this->command = command;
    this->output = output;
	this->function_return = function_return;
}

execute::~execute() {
}

void execute::go_for_it() {
    int file_des[2];
    if (pipe(file_des) == -1) {
		*function_return = errno;
        return;
    }

    pid_t child_pid = fork();

	if(child_pid == -1) {
		*function_return = errno;
		return;
	} else if (child_pid == 0) {
		//this part gets executed only by the child process
		while ((dup2(file_des[1], STDOUT_FILENO) == -1) && (errno == EINTR)) {}
        while ((dup2(file_des[1], STDERR_FILENO) == -1) && (errno == EINTR)) {}

	  	//char *argv[] = {"/usr/bin/timeout", "10", "/bin/sh", "-c", (char *)command.c_str(), 0};
		char const * const argv[] = {"/usr/bin/timeout", "10", "/bin/sh", "-c", (char *)command.c_str(), 0};
		execve(argv[0], (char**)argv, (char * const *)NULL);

        //this part here will only execute if execve failed
		exit(0);
    } else {
		close(file_des[1]);

		char pipe_output[1024];
		ssize_t read_bytes;
		while ((read_bytes = read(file_des[0], pipe_output, 1023)) != 0) {
			if (read_bytes == -1) {
		        if (errno == EINTR)
		            continue;
		        else {
		            *function_return = errno;
		    		return;
		        }
			} else
				output->append(pipe_output, 0, read_bytes);
		}
	}
}
