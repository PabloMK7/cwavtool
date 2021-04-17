#ifndef CWAV_H
#define CWAV_H

#include "../types.h"

#include "imaadpcm/adpcm-lib.h"
#include "dspadpcm/grok.h"

enum {
    CWAV_ENCODING_PCM8 = 0,
    CWAV_ENCODING_PCM16 = 1,
    CWAV_ENCODING_DSP_ADPCM = 2,
    CWAV_ENCODING_IMA_ADPCM = 3
};

typedef struct {
    u32 channels;
    u32 sampleRate;
    u32 bitsPerSample;

    u32 encoding;

    IMAADPCMInfo* imainfos;
    IMAADPCMInfo* imainfosloop;

    DSPADPCMInfo* dspinfos;

    bool loop;
    u32 loopStartFrame;
    u32 loopEndFrame;

    u32 dataSize;
    void* data;
} CWAV;

void* cwav_build(u32* size, CWAV* wav);

#endif