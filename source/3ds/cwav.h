#ifndef CWAV_H
#define CWAV_H

#include "../types.h"

typedef struct {
    u32 channels;
    u32 sampleRate;
    u32 bitsPerSample;
    u32 dataSize;
    u8* data;
} CWAV;

u8* cwav_build(CWAV wav, u32* size);

#endif