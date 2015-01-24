#include "cwav.h"

#include <stdlib.h>
#include <string.h>

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

u8* build_cwav(WAV wav, u32* size) {
    Header header;
    u32 offset = sizeof(Header);

    header.infoChunkOffset = offset;
    header.infoChunkLength = sizeof(InfoHeader);
    offset += header.infoChunkLength;

    offset += (sizeof(ChannelDataPointer) + sizeof(ChannelData)) * wav.format.numChannels;

    header.dataChunkOffset = offset;
    header.dataChunkLength = (u32) sizeof(DataHeader) + wav.data.chunkSize;
    offset += header.dataChunkLength;

    header.fileSize = offset;

    u8* output = (u8*) malloc(offset);
    u32 pos = 0;

    memcpy(output + pos, &header, sizeof(Header));
    pos += sizeof(Header);

    InfoHeader infoHeader;
    infoHeader.type = wav.format.bitsPerSample == 16 ? 1 : 0;
    infoHeader.sampleRate = wav.format.sampleRate;
    infoHeader.totalSamples = wav.data.chunkSize / (wav.format.bitsPerSample / 8) / wav.format.numChannels;
    infoHeader.totalChannels = wav.format.numChannels;
    memcpy(output + pos, &infoHeader, sizeof(InfoHeader));
    pos += sizeof(InfoHeader);

    for(int i = 0; i < wav.format.numChannels; i++) {
        ChannelDataPointer pointer;
        pointer.offset = 0x4 + (wav.format.numChannels * (u32) sizeof(ChannelDataPointer)) + (i * (u32) sizeof(ChannelData));
        memcpy(output + pos, &pointer, sizeof(ChannelDataPointer));
        pos += sizeof(ChannelDataPointer);
    }

    for(int i = 0; i < wav.format.numChannels; i++) {
        ChannelData data;
        data.offset = i * (wav.data.chunkSize / wav.format.numChannels);
        memcpy(output + pos, &data, sizeof(ChannelData));
        pos += sizeof(ChannelData);
    }

    DataHeader dataHeader;
    dataHeader.length = (u32) sizeof(DataHeader) + wav.data.chunkSize;
    memcpy(output + pos, &dataHeader, sizeof(DataHeader));
    pos += sizeof(DataHeader);

    u32 bytesPerChannel = wav.data.chunkSize / wav.format.numChannels;
    u32 bytesPerSample = (u32) wav.format.bitsPerSample / 8;
    for(int i = 0; i < wav.data.chunkSize; i += wav.format.numChannels * bytesPerSample) {
        for(int c = 0; c < wav.format.numChannels; c++) {
            memcpy(output + pos + (bytesPerChannel * c) + (i / wav.format.numChannels), wav.dataBytes + i + (c * bytesPerSample), bytesPerSample);
        }
    }

    pos += wav.data.chunkSize;

    if(size != NULL) {
        *size = pos;
    }

    return output;
}