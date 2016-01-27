#include "util.h"

#include "../pc/stb_image.h"

#include <stdlib.h>

u8 TILE_ORDER[64] = { 0,  1,  8,  9,  2,  3,  10, 11, 16, 17, 24, 25, 18, 19, 26, 27,
        4,  5,  12, 13, 6,  7,  14, 15, 20, 21, 28, 29, 22, 23, 30, 31,
        32, 33, 40, 41, 34, 35, 42, 43, 48, 49, 56, 57, 50, 51, 58, 59,
        36, 37, 44, 45, 38, 39, 46, 47, 52, 53, 60, 61, 54, 55, 62, 63 };

void utf8_to_utf16(u16* dst, const char* src, size_t max_len) {
    size_t n = 0;
    while(src[n]) {
        dst[n] = (u16) src[n];
        if(n++ >= max_len) {
            return;
        }
    }
}

u16 pack_color(u8 r, u8 g, u8 b, u8 a, PixelFormat format) {
    if(format == RGB565) {
        float alpha = a / 255.0f;
        r = (u8) (r * alpha) >> 3;
        g = (u8) (g * alpha) >> 2;
        b = (u8) (b * alpha) >> 3;
        return (r << 11) | (g << 5) | b;
    } else if(format == RGBA4444) {
        r >>= 4;
        g >>= 4;
        b >>= 4;
        a >>= 4;
        return r << 12 | g << 8 | b << 4 | a;
    }

    return 0;
}

u8* load_image(const char* image, u32 width, u32 height) {
    unsigned char *img;
    int imgWidth, imgHeight, imgDepth;

    img = stbi_load(image, &imgWidth, &imgHeight, &imgDepth, STBI_rgb_alpha);

    if(img == NULL) {
        printf("ERROR: Could not load image file: %s.\n", stbi_failure_reason());
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
        stbi_image_free(img);
        return NULL;
    }

    if(imgDepth != STBI_rgb_alpha) {
        printf("ERROR: Decoded image does't match expected format (%d, wanted %d).\n",
            imgDepth, STBI_rgb_alpha);
        stbi_image_free(img);
        return NULL;
    }

    return img;
}

void free_image(u8* img) {
    stbi_image_free(img);
}

u16* image_data_to_tiles(u8* img, u32 width, u32 height, PixelFormat format, u32* size) {
    u16* converted = (u16*) malloc(width * height * sizeof(u16));
    u32 n = 0;
    for(int y = 0; y < height; y += 8) {
        for(int x = 0; x < width; x += 8) {
            for(int k = 0; k < 8 * 8; k++) {
                u32 xx = (u32) (TILE_ORDER[k] & 0x7);
                u32 yy = (u32) (TILE_ORDER[k] >> 3);

                u8* pixel = img + (((y + yy) * width + (x + xx)) * 4);
                converted[n++] = pack_color(pixel[0], pixel[1], pixel[2], pixel[3], format);
            }
        }
    }

    if(size != NULL) {
        *size = width * height * (u32) sizeof(u16);
    }

    return converted;
}

u16* image_to_tiles(const char* image, u32 width, u32 height, PixelFormat format, u32* size) {
    u8* img = load_image(image, width, height);
    if(img == NULL) {
        return NULL;
    }

    u16* tiles = image_data_to_tiles(img, width, height, format, size);
    free_image(img);
    return tiles;
}