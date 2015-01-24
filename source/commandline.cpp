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
        printf("  -i/--image: PNG file to use as the banner's image. Interchangeable with -ci.\n");
        printf("  -a/--audio: WAV file to use as the banner's tune. Interchangeable with -ca.\n");
        printf("  -ci/--cgfximage: CGFX file to use as the banner's image. Interchangeable with -i.\n");
        printf("  -ca/--cwavaudio: CWAV file to use as the banner's tune. Interchangeable with -a.\n");
        printf("  -o/--output: File to output the created banner to.\n");
    } else if(strcmp(command, "makesmdh") == 0) {
        printf("makesmdh - Creates a .smdh/.icn file.\n");
        printf("  -s/--shortdescription: Short description of the application.\n");
        printf("  -l/--longdescription: Long description of the application.\n");
        printf("  -p/--publisher: Publisher of the application.\n");
        printf("  -i/--icon: PNG file to use as an icon.\n");
        printf("  -o/--output: File to output the created SMDH/ICN to.\n");
    } else if(strcmp(command, "makecwav") == 0) {
        printf("makecwav - Creates a CWAV file from a WAV.\n");
        printf("  -i/--input: WAV file to convert.\n");
        printf("  -o/--output: File to output the created CWAV to.\n");
    } else if(strcmp(command, "lz11") == 0) {
        printf("lz11 - Compresses a file with LZ11.\n");
        printf("  -i/--input: File to compress.\n");
        printf("  -o/--output: File to output the compressed data to.\n");
    }
}

void cmd_print_commands() {
    printf("Available commands:\n");
    cmd_print_info("makebanner");
    cmd_print_info("makesmdh");
    cmd_print_info("makecwav");
    cmd_print_info("lz11");
}

void cmd_missing_args(const char* command) {
    printf("Missing arguments for command \"%s\".\n", command);
    cmd_print_info(command);
}

void cmd_invalid_command(const char* command) {
    printf("Invalid command \"%s\".\n", command);
    cmd_print_commands();
}