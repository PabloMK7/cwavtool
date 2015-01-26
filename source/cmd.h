#ifndef __CMD_H__
#define __CMD_H__

#include <string.h>

#include <map>

struct compare_strings {
    bool operator()(char const *a, char const *b) {
        return strcmp(a, b) < 0;
    }
};

std::map<char*, char*, compare_strings> cmd_get_args(int argc, char* argv[]);
char* cmd_find_arg(std::map<char*, char*, compare_strings> args, const char* shortOpt, const char* longOpt);
void cmd_print_usage(const char* executedFrom);
void cmd_print_info(const char* command);
void cmd_print_commands();
void cmd_missing_args(const char* command);
void cmd_invalid_command(const char* command);
int cmd_process_command(int argc, char* argv[]);

#endif