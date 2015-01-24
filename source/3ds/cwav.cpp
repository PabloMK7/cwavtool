#include "cwav.h"

#include "../wav.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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