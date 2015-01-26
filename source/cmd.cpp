#include "cmd.h"

#include "3ds/3ds.h"
#include "pc/wav.h"
#include "types.h"

u8* convert_to_cgfx(const char* image, u32 width, u32 height, u32* size) {
    u32 convertedSize = 0;
    u16* converted = image_to_tiles(image, width, height, RGBA4444, &convertedSize);
    if(converted == NULL) {
        return NULL;
    }

    u8* ret = (u8*) malloc(BANNER_CGFX_HEADER_LENGTH + convertedSize);
    memcpy(ret, BANNER_CGFX_HEADER, BANNER_CGFX_HEADER_LENGTH);
    memcpy(ret + BANNER_CGFX_HEADER_LENGTH, converted, convertedSize);

    *size = BANNER_CGFX_HEADER_LENGTH + convertedSize;
    return ret;
}

u8* convert_to_cwav(const char* file, u32* size) {
    WAV* wav = wav_read(file);
    if(!wav) {
        return NULL;
    }

    CWAV cwav;
    cwav.channels = wav->format.numChannels;
    cwav.sampleRate = wav->format.sampleRate;
    cwav.bitsPerSample = wav->format.bitsPerSample;
    cwav.dataSize = wav->data.chunkSize;
    cwav.data = wav->data.data;

    u8* ret = cwav_build(cwav, size);

    wav_free(wav);

    return ret;
}

int cmd_make_banner(const char* image, const char* audio, char* cgfxFile, char* cwavFile, const char* output) {
    u32 cgfxSize = 0;
    u8* cgfx = NULL;
    if(cgfxFile != NULL) {
        FILE* fd = fopen(cgfxFile, "r");
        if(!fd) {
            printf("ERROR: Could not open CGFX file: %s\n", strerror(errno));
        }

        fseek(fd, 0, SEEK_END);
        cgfxSize = (u32) ftell(fd);
        fseek(fd, 0, SEEK_SET);

        cgfx = (u8*) malloc(cgfxSize);
        fread(cgfx, 1, cgfxSize, fd);
        fclose(fd);
    } else {
        cgfx = convert_to_cgfx(image, 256, 128, &cgfxSize);
        if(!cgfx) {
            return 1;
        }
    }

    u32 cwavSize = 0;
    u8* cwav = NULL;
    if(cwavFile != NULL) {
        FILE* fd = fopen(cwavFile, "r");
        if(!fd) {
            printf("ERROR: Could not open CWAV file: %s\n", strerror(errno));
        }

        fseek(fd, 0, SEEK_END);
        cwavSize = (u32) ftell(fd);
        fseek(fd, 0, SEEK_SET);

        cwav = (u8*) malloc(cwavSize);
        fread(cwav, 1, cwavSize, fd);
        fclose(fd);
    } else {
        cwav = convert_to_cwav(audio, &cwavSize);
        if(!cwav) {
            return 2;
        }
    }

    CBMD cbmd;
    cbmd.cgfxs[CGFX_COMMON] = cgfx;
    cbmd.cgfxSizes[CGFX_COMMON] = cgfxSize;
    cbmd.cwav = cwav;
    cbmd.cwavSize = cwavSize;

    u32 bnrSize = 0;
    u8* bnr = bnr_build(cbmd, &bnrSize);
    free(cgfx);
    free(cwav);

    FILE* fd = fopen(output, "wb");
    if(!fd) {
        printf("ERROR: Could not open output file: %s\n", strerror(errno));
        return 3;
    }

    fwrite(bnr, 1, bnrSize, fd);
    fclose(fd);

    free(bnr);

    printf("Created banner \"%s\".\n", output);
    return 0;
}

int cmd_make_smdh(char* shortDescription, char* longDescription, char* publisher, char* icon, char* output) {
    u16* icon48 = image_to_tiles(icon, 48, 48, RGB565, NULL);
    if(icon48 == NULL) {
        return 1;
    }

    u16 icon24[24 * 24];
    for(int y = 0; y < 24; y++) {
        for(int x = 0; x < 24; x++) {
            icon24[y * 24 + x] = icon48[y * 48 + x];
        }
    }

    SMDH smdh;
    for(int i = 0; i < 0x10; i++) {
        utf8_to_utf16(smdh.titles[i].shortDescription, shortDescription, 0x40);
        utf8_to_utf16(smdh.titles[i].longDescription, longDescription, 0x80);
        utf8_to_utf16(smdh.titles[i].publisher, publisher, 0x40);
    }

    memcpy(smdh.largeIcon, icon48, 0x1200);
    memcpy(smdh.smallIcon, icon24, 0x480);
    free(icon48);

    FILE* fd = fopen(output, "wb");
    if(!fd) {
        printf("ERROR: Could not open output file: %s\n", strerror(errno));
        return 2;
    }

    fwrite(&smdh, 1, sizeof(SMDH), fd);
    fclose(fd);

    printf("Created SMDH \"%s\".\n", output);
    return 0;
}

int cmd_make_cwav(char* input, char* output) {
    u32 cwavSize = 0;
    u8* cwav = convert_to_cwav(input, &cwavSize);
    if(!cwav) {
        return 1;
    }

    FILE* fd = fopen(output, "wb");
    if(!fd) {
        printf("ERROR: Could not open output file: %s\n", strerror(errno));
        return 2;
    }

    fwrite(cwav, 1, cwavSize, fd);
    fclose(fd);

    free(cwav);

    printf("Created CWAV \"%s\".\n", output);
    return 0;
}

int cmd_lz11(char* input, char* output) {
    FILE* in = fopen(input, "r");
    if(!in) {
        printf("ERROR: Could not open input file: %s\n", strerror(errno));
        return 1;
    }

    fseek(in, 0, SEEK_END);
    u32 size = (u32) ftell(in);
    fseek(in, 0, SEEK_SET);

    u8 data[size];
    fread(data, 1, size, in);
    fclose(in);

    u32 compressedSize;
    u8* compressed = lz11_compress(data, size, &compressedSize);
    if(!compressed) {
        return 2;
    }

    FILE* fd = fopen(output, "wb");
    if(!fd) {
        printf("ERROR: Could not open output file: %s\n", strerror(errno));
        return 3;
    }

    fwrite(compressed, 1, compressedSize, fd);
    fclose(fd);

    free(compressed);

    printf("Compressed to file \"%s\".\n", output);
    return 0;
}

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

int cmd_process_command(int argc, char* argv[]) {
    if(argc < 2) {
        cmd_print_usage(argv[0]);
        return -1;
    }

    char* command = argv[1];
    std::map<char*, char*, compare_strings> args = cmd_get_args(argc, argv);
    if(strcmp(command, "makebanner") == 0) {
        char *banner = cmd_find_arg(args, "i", "image");
        char *audio = cmd_find_arg(args, "a", "audio");
        char *cgfxFile = cmd_find_arg(args, "ci", "cgfximage");
        char *cwavFile = cmd_find_arg(args, "ca", "cwavaudio");
        char *output = cmd_find_arg(args, "o", "output");
        if(!(banner || cgfxFile) || !(audio || cwavFile) || !output) {
            cmd_missing_args(command);
            return -1;
        }

        return cmd_make_banner(banner, audio, cgfxFile, cwavFile, output);
    } else if(strcmp(command, "makesmdh") == 0) {
        char* shortDescription = cmd_find_arg(args, "s", "shortdescription");
        char* longDescription = cmd_find_arg(args, "l", "longdescription");
        char* publisher = cmd_find_arg(args, "p", "publisher");
        char* icon = cmd_find_arg(args, "i", "icon");
        char* output = cmd_find_arg(args, "o", "output");
        if(!shortDescription || !longDescription || !publisher || !icon || !output) {
            cmd_missing_args(command);
            return -1;
        }

        return cmd_make_smdh(shortDescription, longDescription, publisher, icon, output);
    } else if(strcmp(command, "makecwav") == 0) {
        char* input = cmd_find_arg(args, "i", "input");
        char* output = cmd_find_arg(args, "o", "output");
        if(!input || !output) {
            cmd_missing_args(command);
            return -1;
        }

        return cmd_make_cwav(input, output);
    } else if(strcmp(command, "lz11") == 0) {
        char* input = cmd_find_arg(args, "i", "input");
        char* output = cmd_find_arg(args, "o", "output");
        if(!input || !output) {
            cmd_missing_args(command);
            return -1;
        }

        return cmd_lz11(input, output);
    } else {
        cmd_invalid_command(command);
        return -1;
    }
}