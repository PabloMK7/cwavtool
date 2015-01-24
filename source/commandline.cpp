#include "commandline.h"

std::map<char*, char*, compare_strings> cmd_get_args(int argc, char* argv[]) {
    std::map<char*, char*, compare_strings> args;
    for(int i = 0; i < argc; i++) {
        if((strncmp(argv[i], "-", 1) == 0 || strncmp(argv[i], "--", 2) == 0) && argc != i + 1) {
            args.insert(std::pair<char*, char*>(argv[i], argv[i + 1]));
            i++;
        }
    }

    return args;
}

char* cmd_find_arg(std::map<char*, char*, compare_strings> args, const char* shortOpt, const char* longOpt) {
    char sopt[strlen(shortOpt) + 2];
    sprintf(sopt, "-%s", shortOpt);
    char lopt[strlen(longOpt) + 3];
    sprintf(lopt, "--%s", longOpt);

    std::map<char*, char*, compare_strings>::iterator match = args.find(sopt);
    if(match != args.end()) {
        return (*match).second;
    }

    match = args.find(lopt);
    if(match != args.end()) {
        return (*match).second;
    }

    return NULL;
}

void cmd_print_usage(const char* executedFrom) {
    printf("Usage: %s <command> <args>\n", executedFrom);
    cmd_print_commands();
}

void cmd_print_info(const char* command) {
    if(strcmp(command, "makebanner") == 0) {
        printf("makebanner - Creates a .bnr file.\n");
        printf("  -i/--image: PNG file to use as the banner image.\n");
        printf("  -a/--audio: Audio file to use as the banner's tune.\n");
        printf("  -o/--output: File to output the created banner to.\n");
    }
}

void cmd_print_commands() {
    printf("Available commands:\n");
    cmd_print_info("makebanner");
}

void cmd_missing_args(const char* command) {
    printf("Missing arguments for command \"%s\".\n", command);
    cmd_print_info(command);
}

void cmd_invalid_command(const char* command) {
    printf("Invalid command \"%s\".\n", command);
    cmd_print_commands();
}