#ifndef WAV_H
#define WAV_H

#include <stdio.h>

#include "../types.h"

typedef struct {
    char chunkId[4];
    u32 chunkSize;
    char format[4];
} Riff;

typedef struct {
    char chunkId[4];
    u32 chunkSize;
    u16 format;
    u16 numChannels;
    u32 sampleRate;
    u32 byteRate;
    u16 align;
    u16 bitsPerSample;
} Format;

typedef struct {
    char chunkId[4];
    u32 chunkSize;
    u8* data;
} Data;

typedef struct {
    Riff riff;
    Format format;
    Data data;
} WAV;

WAV* wav_read(FILE* fd);
void wav_free(WAV* wav);

#endif