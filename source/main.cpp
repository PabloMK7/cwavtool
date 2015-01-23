#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "data.h"
#include "lz11.h"
#include "types.h"
#include "lodepng.h"

u8* convert_to_cgfx(const char* file, u32 width, u32 height, u32* size) {
    unsigned char* img;
    unsigned int imgWidth, imgHeight;
    if(lodepng_decode32_file(&img, &imgWidth, &imgHeight, file)) {
        printf("ERROR: Could not load png file.\n");
        return NULL;
    }

    if(imgWidth != width || imgHeight != height) {
        printf("ERROR: Image must be exactly %d x %d in size.\n", width, height);
        return NULL;
    }

    u8 converted[width * height * 2];
    u32 n = 0;
    for(int y = 0; y < height; y += 8) {
        for(int x = 0; x < width; x += 8) {
            for(int k = 0; k < 8 * 8; k++) {
                u32 xx = (u32) (TILE_ORDER[k] & 0x7);
                u32 yy = (u32) (TILE_ORDER[k] >> 3);

                u8* pixel = img + (((y + yy) * width + (x + xx)) * 4);
                converted[n++] = ((pixel[2] >> 4) << 4) | (pixel[3] >> 4);
                converted[n++] = ((pixel[0] >> 4) << 4) | (pixel[1] >> 4);
            }
        }
    }

    u8* ret = (u8*) malloc(CGFX_HEADER_LENGTH + (width * height * 2));
    memcpy(ret, CGFX_HEADER, CGFX_HEADER_LENGTH);
    memcpy(ret + CGFX_HEADER_LENGTH, converted, width * height * 2);

    *size = CGFX_HEADER_LENGTH + (width * height * 2);
    return ret;
}

u8* make_banner(const char* file, u32* size) {
    u32 originalSize = 0;
    u8* cgfx = convert_to_cgfx(file, 256, 128, &originalSize);
    if(!cgfx) {
        return NULL;
    }

    u32 compressedSize = 0;
    u8* compressed = compress_lz11(cgfx, originalSize, &compressedSize);
    free(cgfx);
    if(!compressed) {
        return NULL;
    }

    u32 pad = 16 - ((BANNER_CBMD_HEADER_LENGTH + 4 + compressedSize) % 16);
    u32 totalLength = BANNER_CBMD_HEADER_LENGTH + 4 + compressedSize + pad;

    u8* ret = (u8*) malloc(totalLength);
    memcpy(ret, BANNER_CBMD_HEADER, BANNER_CBMD_HEADER_LENGTH);
    memcpy(ret + BANNER_CBMD_HEADER_LENGTH, &totalLength, 4);
    memcpy(ret + BANNER_CBMD_HEADER_LENGTH + 4, compressed, compressedSize);
    memset(ret + BANNER_CBMD_HEADER_LENGTH + 4 + compressedSize, 0, pad);

    free(compressed);
    *size = (u32) totalLength;
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

int main(int argc, char* argv[]) {
    if(argc != 4) {
        printf("Usage: %s <banner png> <audio bcwav> <output file>", argv[0]);
    }

    u32 bannerSize = 0;
    u8* banner = make_banner(argv[1], &bannerSize);
    if(!banner) {
        return 1;
    }

    u32 audioSize = 0;
    u8* audio = make_audio(argv[2], &audioSize);
    if(!audio) {
        return 2;
    }

    FILE* fd = fopen(argv[3], "wb");
    if(!fd) {
        printf("ERROR: Could not write output file.\n");
        return 3;
    }

    fwrite(banner, 1, bannerSize, fd);
    fwrite(audio, 1, audioSize, fd);
    fclose(fd);
    return 0;
}