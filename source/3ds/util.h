#ifndef __UTIL_H__
#define __UTIL_H__

#include "../types.h"

u16 rgba_to_rgb565(u8 r, u8 g, u8 b, u8 a);
u16* image_to_tiles(const char* image, u32 width, u32 height, u32* size);
void utf8_to_utf16(u16* dst, const char* src, size_t max_len);

#endif