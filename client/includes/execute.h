#ifndef EXECUTE_H
#define EXECUTE_H

#include <iostream>
#include <string.h>

class execute
{
    public:
        execute(std::string, std::string *, unsigned short *);
        virtual ~execute();
        void go_for_it();
    private:
        std::string command;
        std::string *output;
		unsigned short *function_return;
};

#endif // EXECUTE_H
