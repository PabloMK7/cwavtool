#include "data.h"
#include "3ds/3ds.h"

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

u8* make_audio(const char* file, u32* size) {
    // TODO: convert from a WAV file.
    FILE* fd = fopen(file, "rb");
    if(!fd) {
        printf("ERROR: Could not load audio file.\n");
        return NULL;
    }

    fseek(fd, 0, SEEK_END);
    size_t length = (size_t) ftell(fd);
    fseek(fd, 0, SEEK_SET);

    u8* data = (u8*) malloc(length);
    fread(data, 1, length, fd);
    fclose(fd);

    *size = (u32) length;
    return data;
}

int make_banner(const char* image, const char* audio, const char* output) {
    u32 cgfxSize = 0;
    u8* cgfx = convert_to_cgfx(image, 256, 128, &cgfxSize);
    if(!cgfx) {
        return 1;
    }

    u32 cwavSize = 0;
    u8* cwav = make_audio(audio, &cwavSize);
    if(!audio) {
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
        printf("ERROR: Could not open output file.\n");
        return 3;
    }

    fwrite(bnr, 1, bnrSize, fd);
    fclose(fd);
    free(bnr);
    return 0;
}

int main(int argc, char* argv[]) {
    if(argc != 4) {
        printf("Usage: %s <banner png> <audio bcwav> <output file>", argv[0]);
    }

    return make_banner(argv[1], argv[2], argv[3]);
}