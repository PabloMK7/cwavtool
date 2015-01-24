#include "util.h"

#include "../lodepng/lodepng.h"

u8 TILE_ORDER[64] = { 0,  1,  8,  9,  2,  3,  10, 11, 16, 17, 24, 25, 18, 19, 26, 27,
        4,  5,  12, 13, 6,  7,  14, 15, 20, 21, 28, 29, 22, 23, 30, 31,
        32, 33, 40, 41, 34, 35, 42, 43, 48, 49, 56, 57, 50, 51, 58, 59,
        36, 37, 44, 45, 38, 39, 46, 47, 52, 53, 60, 61, 54, 55, 62, 63 };

u8* image_to_tiles(const char* image, u32 width, u32 height, u32* size) {
    unsigned char* img;
    unsigned int imgWidth, imgHeight;
    if(lodepng_decode32_file(&img, &imgWidth, &imgHeight, image)) {
        printf("ERROR: Could not load png file.\n");
        return NULL;
    }

    if(width == 0) {
        width = imgWidth;
    }

    if(height == 0) {
        height = imgHeight;
    }

    if(imgWidth != width || imgHeight != height) {
        printf("ERROR: Image must be exactly %d x %d in size.\n", width, height);
        return NULL;
    }

    u8* converted = (u8*) malloc(width * height * 2);
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

    if(size != NULL) {
        *size = width * height * 2;
    }

    return converted;
}