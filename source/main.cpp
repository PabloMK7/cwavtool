#include "3ds/3ds.h"
#include "commandline.h"
#include "data.h"
#include "types.h"
#include "wav.h"

#include <stdio.h>
#include <malloc.h>
#include <string.h>

u8* convert_to_cgfx(const char* image, u32 width, u32 height, u32* size) {
    u32 convertedSize = 0;
    u8* converted = image_to_tiles(image, width, height, &convertedSize);
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
    WAV* wav = read_wav(file);
    if(!wav) {
        return NULL;
    }

    u8* cwav = build_cwav(*wav, size);

    free(wav->dataBytes);
    free(wav);

    return cwav;
}

int make_banner(const char* image, const char* audio, const char* output) {
    u32 cgfxSize = 0;
    u8* cgfx = convert_to_cgfx(image, 256, 128, &cgfxSize);
    if(!cgfx) {
        return 1;
    }

    u32 cwavSize = 0;
    u8* cwav = convert_to_cwav(audio, &cwavSize);
    if(!cwav) {
        return 2;
    }

    CBMD cbmd;
    cbmd.cgfxs[CGFX_COMMON] = cgfx;
    cbmd.cgfxSizes[CGFX_COMMON] = cgfxSize;
    cbmd.cwav = cwav;
    cbmd.cwavSize = cwavSize;

    u32 bnrSize = 0;
    u8* bnr = build_bnr(cbmd, &bnrSize);
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

int make_cwav(char* input, char* output) {
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

int lz11(char* input, char* output) {
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

int main(int argc, char* argv[]) {
    if(argc < 2) {
        cmd_print_usage(argv[0]);
        return -1;
    }

    char* command = argv[1];
    std::map<char*, char*, compare_strings> args = cmd_get_args(argc, argv);
    if(strcmp(command, "makebanner") == 0) {
        char* banner = cmd_find_arg(args, "i", "image");
        char* audio = cmd_find_arg(args, "a", "audio");
        char* output = cmd_find_arg(args, "o", "output");
        if(!banner || !audio || !output) {
            cmd_missing_args(command);
            return -1;
        }

        return make_banner(banner, audio, output);
    } else if(strcmp(command, "makecwav") == 0) {
        char* input = cmd_find_arg(args, "i", "input");
        char* output = cmd_find_arg(args, "o", "output");
        if(!input || !output) {
            cmd_missing_args(command);
            return -1;
        }

        return make_cwav(input, output);
    } else if(strcmp(command, "lz11") == 0) {
        char* input = cmd_find_arg(args, "i", "input");
        char* output = cmd_find_arg(args, "o", "output");
        if(!input || !output) {
            cmd_missing_args(command);
            return -1;
        }

        return lz11(input, output);
    } else {
        cmd_invalid_command(command);
        return -1;
    }
}