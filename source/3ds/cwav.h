#ifndef __CWAV_H__
#define __CWAV_H__

#include "../types.h"
#include "../wav.h"

typedef struct {
    char magic[4] = {'C', 'W', 'A', 'V'};
    u16 endianess = 0xFEFF;
    u16 structLength = 0x40;
    u32 unknown0 = 0;
    u32 fileSize;
    u32 numChunks = 2;
    u32 infoChunkFlags = 0x7000;
    u32 infoChunkOffset;
    u32 infoChunkLength;
    u32 dataChunkFlags = 0x7000;
    u32 dataChunkOffset;
    u32 dataChunkLength;
    u8 reserved[0x14] = {0};
} Header;

typedef struct {
    char magic[4] = {'I', 'N', 'F', 'O'};
    u32 length = 0xC0;
    u32 type;
    u32 sampleRate;
    u32 unknown1 = 0;
    u32 totalSamples;
    u32 unknown2 = 0;
    u32 totalChannels;
} InfoHeader;

typedef struct {
    char magic[4] = {'D', 'A', 'T', 'A'};
    u32 length;
} DataHeader;

typedef struct {
    u32 flags = 0x7100;
    u32 offset;
} ChannelDataPointer;

typedef struct {
    u32 flags = 0x1F00;
    u32 offset;
    u32 unknown3 = 0;
    u32 unknown4 = 0;
    u32 padding = 0;
} ChannelData;

u8* build_cwav(WAV wav, u32* size);

#endif