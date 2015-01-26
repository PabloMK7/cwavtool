#include "cwav.h"

#include <stdlib.h>
#include <string.h>

typedef enum {
    PCM8,
    PCM16,
    TODO
} CWAVType;

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
} CWAVHeader;

typedef struct {
    char magic[4] = {'I', 'N', 'F', 'O'};
    u32 length = 0xC0;
    u32 type;
    u32 sampleRate;
    u32 unknown1 = 0;
    u32 totalSamples;
    u32 unknown2 = 0;
    u32 totalChannels;
} CWAVInfoHeader;

typedef struct {
    u32 flags = 0x7100;
    u32 offset;
} CWAVChannelDataPointer;

typedef struct {
    u32 flags = 0x1F00;
    u32 offset;
    u32 unknown3 = 0;
    u32 unknown4 = 0;
    u32 padding = 0;
} CWAVChannelData;

typedef struct {
    char magic[4] = {'D', 'A', 'T', 'A'};
    u32 length;
} CWAVDataHeader;

u8* cwav_build(CWAV cwav, u32* size) {
    CWAVHeader header;
    u32 offset = sizeof(CWAVHeader);

    header.infoChunkOffset = offset;
    header.infoChunkLength = sizeof(CWAVInfoHeader);
    offset += header.infoChunkLength;

    offset += (sizeof(CWAVChannelDataPointer) + sizeof(CWAVChannelData)) * cwav.channels;

    header.dataChunkOffset = offset;
    header.dataChunkLength = (u32) sizeof(CWAVDataHeader) + cwav.dataSize;
    offset += header.dataChunkLength;

    header.fileSize = offset;

    u8* output = (u8*) malloc(offset);
    u32 pos = 0;

    memcpy(output + pos, &header, sizeof(CWAVHeader));
    pos += sizeof(CWAVHeader);

    u32 bytesPerSample = (u32) cwav.bitsPerSample / 8;

    CWAVInfoHeader infoHeader;
    infoHeader.type = cwav.bitsPerSample == 16 ? PCM16 : PCM8;
    infoHeader.sampleRate = cwav.sampleRate;
    infoHeader.totalSamples = cwav.dataSize / bytesPerSample / cwav.channels;
    infoHeader.totalChannels = cwav.channels;
    memcpy(output + pos, &infoHeader, sizeof(CWAVInfoHeader));
    pos += sizeof(CWAVInfoHeader);

    for(int i = 0; i < cwav.channels; i++) {
        CWAVChannelDataPointer pointer;
        pointer.offset = 0x4 + (cwav.channels * (u32) sizeof(CWAVChannelDataPointer)) + (i * (u32) sizeof(CWAVChannelData));
        memcpy(output + pos, &pointer, sizeof(CWAVChannelDataPointer));
        pos += sizeof(CWAVChannelDataPointer);
    }

    u32 bytesPerChannel = cwav.dataSize / cwav.channels;
    for(int i = 0; i < cwav.channels; i++) {
        CWAVChannelData data;
        data.offset = i * bytesPerChannel;
        memcpy(output + pos, &data, sizeof(CWAVChannelData));
        pos += sizeof(CWAVChannelData);
    }

    CWAVDataHeader dataHeader;
    dataHeader.length = (u32) sizeof(CWAVDataHeader) + cwav.dataSize;
    memcpy(output + pos, &dataHeader, sizeof(CWAVDataHeader));
    pos += sizeof(CWAVDataHeader);

    for(int i = 0; i < cwav.dataSize; i += cwav.channels * bytesPerSample) {
        for(int c = 0; c < cwav.channels; c++) {
            memcpy(output + pos + (bytesPerChannel * c) + (i / cwav.channels), cwav.data + i + (c * bytesPerSample), bytesPerSample);
        }
    }

    pos += cwav.dataSize;

    if(size != NULL) {
        *size = pos;
    }

    return output;
}