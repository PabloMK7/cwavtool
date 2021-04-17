#include "cwav.h"


#include <stdlib.h>
#include <string.h>

enum {
    CWAV_REF_DSP_ADPCM_INFO = 0x0300,
    CWAV_REF_IMA_ADPCM_INFO = 0x0301,
    CWAV_REF_SAMPLE_DATA = 0x1F00,
    CWAV_REF_INFO_BLOCK = 0x7000,
    CWAV_REF_DATA_BLOCK = 0x7001,
    CWAV_REF_CHANNEL_INFO = 0x7100
};

typedef struct {
    u16 typeId;
    u16 padding;
    u32 offset;
} CWAVReference;

typedef struct {
    CWAVReference ref;
    u32 size;
} CWAVSizedReference;

typedef struct {
    u32 count;
    CWAVReference contents[0]; // Relative to beginning of CWAVReferenceTable.
} CWAVReferenceTable;

#define CWAV_MAGIC "CWAV"

enum {
    CWAV_ENDIANNESS_LITTLE = 0xFEFF,
    CWAV_ENDIANNESS_BIG = 0xFFFE
};

#define CWAV_VERSION 0x02010000

typedef struct {
    char magic[4];
    u16 endianness;
    u16 headerSize;
    u32 version;
    u32 fileSize;
    u16 numBlocks;
    u16 reserved;
    CWAVSizedReference infoBlock; // Relative to start of file.
    CWAVSizedReference dataBlock; // Relative to start of file.
} CWAVHeader;

#define CWAV_BLOCK_MAGIC_INFO "INFO"
#define CWAV_BLOCK_MAGIC_DATA "DATA"

typedef struct {
    char magic[4];
    u32 size;
} CWAVBlockHeader;

typedef struct {
    CWAVBlockHeader header;
    u8 encoding;
    bool loop;
    u16 padding;
    u32 sampleRate;
    u32 loopStartFrame;
    u32 loopEndFrame;
    u32 reserved;
    CWAVReferenceTable channelInfos;
} CWAVInfoBlockHeader;

typedef struct {
    CWAVReference samples; // Relative to CWAVDataBlock.data
    CWAVReference adpcmInfo; // Relative to beginning of CWAVChannelInfo.
    u32 reserved;
} CWAVChannelInfo;

typedef struct {
    u16 coefficients[16];
} CWAVDSPADPCMParam;

typedef struct {
    u8 predictorScale;
    u8 reserved;
    u16 previousSample;
    u16 secondPreviousSample;
} CWAVDSPADPCMContext;

typedef struct {
    CWAVDSPADPCMParam param;
    CWAVDSPADPCMContext context;
    CWAVDSPADPCMContext loopContext;
    u16 padding;
} CWAVDSPADPCMInfo;

typedef struct {
    u16 data;
    u8 tableIndex;
    u8 padding;
} CWAVIMAADPCMContext;

typedef struct {
    CWAVIMAADPCMContext context;
    CWAVIMAADPCMContext loopContext;
} CWAVIMAADPCMInfo;

typedef struct {
    CWAVBlockHeader header;
} CWAVDataBlock;

int cwav_convert_pcm16_pcm16(CWAV* cwav) {
    s16* newData = (s16*)malloc(cwav->loopEndFrame * cwav->channels * 2);
    if (!newData) {
        printf("ERROR: Could not allocate memory for CWAV sample data.\n");
        return -1;
    }
    u32 currChannel = 0;
    u32 samplesPerChannel = cwav->loopEndFrame;
    for (u32 i = 0; i < cwav->loopEndFrame * cwav->channels; i++) {
        s16 sample = ((s16*)(cwav->data))[i];
        newData[samplesPerChannel * currChannel + (i / cwav->channels)] = sample;
        currChannel++;
        if (currChannel >= cwav->channels)
            currChannel = 0;
    }
    free(cwav->data);
    cwav->data = newData;
    cwav->dataSize = cwav->loopEndFrame * cwav->channels * 2;
    return 0;
}

int cwav_convert_pcm16_pcm8(CWAV* cwav) {
    s8* newData = (s8*)malloc(cwav->loopEndFrame * cwav->channels);
    if (!newData) {
        printf("ERROR: Could not allocate memory for CWAV sample data.\n");
        return -1;
    }
    u32 currChannel = 0;
    u32 samplesPerChannel = cwav->loopEndFrame;
    for (u32 i = 0; i < cwav->loopEndFrame * cwav->channels; i++) {
        s16 sample = ((s16*)(cwav->data))[i];
        sample /= 256;
        newData[samplesPerChannel * currChannel + (i / cwav->channels)] = (s8)sample;
        currChannel++;
        if (currChannel >= cwav->channels)
            currChannel = 0;
    }
    free(cwav->data);
    cwav->data = newData;
    cwav->dataSize = cwav->loopEndFrame * cwav->channels;
    return 0;
}

int cwav_convert_pcm8_pcm8(CWAV* cwav) {
    s8* newData = (s8*)malloc(cwav->loopEndFrame * cwav->channels);
    if (!newData) {
        printf("ERROR: Could not allocate memory for CWAV sample data.\n");
        return -1;
    }
    u32 currChannel = 0;
    u32 samplesPerChannel = cwav->loopEndFrame;
    for (u32 i = 0; i < cwav->loopEndFrame * cwav->channels; i++) {
        s8 sample = ((s8*)(cwav->data))[i];
        newData[samplesPerChannel * currChannel + (i / cwav->channels)] = sample;
        currChannel++;
        if (currChannel >= cwav->channels)
            currChannel = 0;
    }
    free(cwav->data);
    cwav->data = newData;
    cwav->dataSize = cwav->loopEndFrame * cwav->channels;
    return 0;
}

int cwav_convert_pcm8_pcm16(CWAV* cwav)
{
    s16* newData = (s16*)malloc(cwav->loopEndFrame * cwav->channels * 2);
    if (!newData) {
        printf("ERROR: Could not allocate memory for CWAV sample data.\n");
        return -1;
    }
    u32 currChannel = 0;
    u32 samplesPerChannel = cwav->loopEndFrame;
    for (u32 i = 0; i < cwav->loopEndFrame * cwav->channels; i++) {
        s16 sample = ((s8*)(cwav->data))[i];
        sample *= 256;
        newData[samplesPerChannel * currChannel + (i / cwav->channels)] = sample;
        currChannel++;
        if (currChannel >= cwav->channels)
            currChannel = 0;
    }
    free(cwav->data);
    cwav->data = newData;
    cwav->dataSize = cwav->loopEndFrame * 2;
    return 0;
}

static inline int roundUp(int numToRound, int multiple) 
{
    return ((numToRound + multiple - 1) / multiple) * multiple;
}

int cwav_align_pcm16(CWAV* cwav, int alignment)
{
    u32 samples = cwav->dataSize / cwav->channels / 2;
    u32 remainingsamples = roundUp(cwav->loopEndFrame, alignment) - cwav->loopEndFrame; // Remaining to next multiple of alignment.

    if (remainingsamples != 0 || samples != cwav->loopEndFrame) { // Only do if not yet aligned or doesn't match loop end frame
        
        // New buffer to copy aligned samples to.
        s16* newBlock = (s16*)malloc((cwav->loopEndFrame + remainingsamples) * cwav->channels * 2);
        if (!newBlock) {
            printf("ERROR: Could not allocate memory for CWAV sample data.\n");
            return -1;
        }

        for (u32 i = 0; i < cwav->channels; i++) {
            s16* dstChnStart = newBlock + (cwav->loopEndFrame + remainingsamples) * i;
            memcpy(dstChnStart, (u8*)cwav->data + samples * i * 2, cwav->loopEndFrame * 2);
            for (u32 j = 0; j < remainingsamples; j++) {
                if (cwav->loop) // Translate the loop point until it matches with multiple of 8
                    *(dstChnStart + cwav->loopEndFrame + j) = *(dstChnStart + cwav->loopStartFrame + j);
                else // Fill the remaining data with silence
                    *(dstChnStart + cwav->loopEndFrame + j) = 0;
            }
        }
        // Finally, translate the loop point values.
        cwav->loopEndFrame += remainingsamples;
        if (cwav->loop) cwav->loopStartFrame += remainingsamples;

        free(cwav->data);
        cwav->data = newBlock;
        cwav->dataSize = cwav->loopEndFrame * cwav->channels * 2;
    }
    
    // Loop start point may not be aligned yet, just align to next multiple of alignment (negigible in playback for small values).
    cwav->loopStartFrame = roundUp(cwav->loopStartFrame, alignment);
    if (cwav->loopStartFrame >= cwav->loopEndFrame) {
        printf("ERROR: Invalid loop range.\n");
        return -1;
    }
    return 0;
}

int cwav_convert_pcm16_imaadpcm(CWAV* cwav) {
    int ret = 0;
    if ((ret = cwav_align_pcm16(cwav, 8)) < 0)
        return ret;
    u32 imasize = cwav->dataSize / 4; // From 16 bit to 4 bit
    u32 samples = cwav->dataSize / cwav->channels / 2;
    u8* newdata = (u8*)malloc(imasize);
    if (!newdata) {
        printf("ERROR: Could not allocate memory for CWAV sample data.\n");
        return -1;
    }
    for (u32 i = 0; i < cwav->channels; i++) {
        s16* chnStart = (s16*)cwav->data + samples * i;
        u8* chnAdpcmStart = newdata + (samples * i) / 2;
        int initial_deltas[2];
        adpcm_calculate_initial_deltas(initial_deltas, 1, chnStart, samples);
        void* ctx = adpcm_create_context(1, 3, NOISE_SHAPING_DYNAMIC, initial_deltas);
        if (!ctx) {
            printf("ERROR: Could not allocate memory for ADPCM context.\n");
            free(newdata);
            return -1;
        } if (cwav->loop) {
            size_t outcnt = 0;
            adpcm_encode_block(ctx, &(cwav->imainfos[i]), chnAdpcmStart, &outcnt, chnStart, cwav->loopStartFrame);
            if (outcnt != cwav->loopStartFrame / 2) {
                printf("ERROR: Failed to encode ADPCM data.\n");
                adpcm_free_context(ctx);
                free(newdata);
                return -1;
            }
            adpcm_encode_block(ctx, &(cwav->imainfosloop[i]), chnAdpcmStart + (cwav->loopStartFrame / 2), &outcnt, chnStart + cwav->loopStartFrame, cwav->loopEndFrame - cwav->loopStartFrame);
            if (outcnt != (cwav->loopEndFrame - cwav->loopStartFrame) / 2) {
                printf("ERROR: Failed to encode ADPCM data.\n");
                adpcm_free_context(ctx);
                free(newdata);
                return -1;
            }
        } else {
            size_t outcnt = 0;
            adpcm_encode_block(ctx, &(cwav->imainfos[i]), chnAdpcmStart, &outcnt, chnStart, cwav->loopEndFrame);
            if (outcnt != cwav->loopEndFrame / 2) {
                printf("ERROR: Failed to encode ADPCM data.\n");
                adpcm_free_context(ctx);
                free(newdata);
                return -1;
            }
            memcpy(&(cwav->imainfosloop[i]), &(cwav->imainfos[i]), sizeof(IMAADPCMInfo));
        }
        adpcm_free_context(ctx);
    }
    free(cwav->data);
    cwav->data = newdata;
    cwav->dataSize = imasize;
    return 0;
}

static inline u32 cwav_dspsamples_to_bytes(u32 samples) {
    return (samples / 14) * 8;
}

int cwav_convert_pcm16_dspadpcm(CWAV* cwav) {
    int ret = 0;
    if ((ret = cwav_align_pcm16(cwav, 14)) < 0)
        return ret;
    u32 samples = cwav->dataSize / cwav->channels / 2;
    u32 dspsize = cwav_dspsamples_to_bytes(samples * cwav->channels);
    u8* newdata = (u8*)malloc(dspsize);
    if (!newdata) {
        printf("ERROR: Could not allocate memory for CWAV sample data.\n");
        return -1;
    }
    for (u32 i = 0; i < cwav->channels; i++) {
        s16* chnStart = (s16*)cwav->data + samples * i;
        u8* dspchnStart = newdata + cwav_dspsamples_to_bytes(samples * i);
        DSPCorrelateCoefs(chnStart, samples, cwav->dspinfos[i].coefs);
        constexpr u32 PACKET_SAMPLES = 14;
        u32 packetCount = samples / PACKET_SAMPLES;
        int16_t convSamps[16] = {0};
        unsigned char block[8];
        cwav->dspinfos[i].info.hist1 = cwav->dspinfos[i].info.hist2 = 0;
        for (u32 p=0 ; p<packetCount ; ++p) {

            memcpy(convSamps + 2, chnStart + p*PACKET_SAMPLES, PACKET_SAMPLES * sizeof(int16_t));

            DSPEncodeFrame(convSamps, PACKET_SAMPLES, block, reinterpret_cast<short (*)[2]>(cwav->dspinfos[i].coefs));
            
            if (p == 0) {
                cwav->dspinfos[i].info.index = block[0];
            }
            if (p == cwav->loopStartFrame / PACKET_SAMPLES) {
                cwav->dspinfos[i].loopinfo.index = block[0];
                cwav->dspinfos[i].loopinfo.hist1 = convSamps[1];
                cwav->dspinfos[i].loopinfo.hist2 = convSamps[0];
            }

            convSamps[0] = convSamps[14];
            convSamps[1] = convSamps[15];

            memcpy(dspchnStart + p * cwav_dspsamples_to_bytes(PACKET_SAMPLES), block, cwav_dspsamples_to_bytes(PACKET_SAMPLES));
        }
    }
    free(cwav->data);
    cwav->data = newdata;
    cwav->dataSize = dspsize;
    return 0;
}

int cwav_convert_target_format(CWAV* cwav) {
    if (cwav->encoding == CWAV_ENCODING_PCM16 && cwav->bitsPerSample == 16)
        return cwav_convert_pcm16_pcm16(cwav);
    else if (cwav->encoding == CWAV_ENCODING_PCM8 && cwav->bitsPerSample == 8)
        return cwav_convert_pcm8_pcm8(cwav);
    else if (cwav->encoding == CWAV_ENCODING_PCM8 && cwav->bitsPerSample == 16)
        return cwav_convert_pcm16_pcm8(cwav);
    else { // Everything needs to be converted to 16 bit at this point.
        int ret;
        if (cwav->bitsPerSample == 8) {
            ret = cwav_convert_pcm8_pcm16(cwav);
            if (ret < 0)
                return ret;
        } else {
            ret = cwav_convert_pcm16_pcm16(cwav);
            if (ret < 0)
                return ret;
        }
        if (cwav->encoding == CWAV_ENCODING_PCM16)
            return ret;
        else if (cwav->encoding == CWAV_ENCODING_IMA_ADPCM)
            return cwav_convert_pcm16_imaadpcm(cwav);
        else if (cwav->encoding == CWAV_ENCODING_DSP_ADPCM)
            return cwav_convert_pcm16_dspadpcm(cwav);
    }
    return -1;
}

void* cwav_build(u32* size, CWAV* cwav) {
    if (cwav_convert_target_format(cwav) < 0) // Conversion and/or de-interleaving.
        return NULL;

    u32 headerSize = (sizeof(CWAVHeader) + 0x1F) & ~0x1F;
    u32 infoSize = (sizeof(CWAVInfoBlockHeader) + (cwav->channels * (sizeof(CWAVReference) + sizeof(CWAVChannelInfo))));
    u32 adpcmOffset = headerSize + infoSize;
    if (cwav->encoding == CWAV_ENCODING_IMA_ADPCM)
        infoSize += sizeof(CWAVIMAADPCMInfo) * cwav->channels;
    if (cwav->encoding == CWAV_ENCODING_DSP_ADPCM)
        infoSize += sizeof(CWAVDSPADPCMInfo) * cwav->channels;
    infoSize = (infoSize + 0x1F) & ~0x1F;
    u32 dataSize = ((sizeof(CWAVDataBlock) + 0x1F) & ~0x1F) + cwav->dataSize;

    u32 outputSize = headerSize + infoSize + dataSize;

    void* output = calloc(outputSize, sizeof(u8));
    if(output == NULL) {
        printf("ERROR: Could not allocate memory for CWAV data.\n");
        return NULL;
    }

    CWAVHeader* header = (CWAVHeader*) &((u8*) output)[0];
    memcpy(header->magic, CWAV_MAGIC, sizeof(header->magic));
    header->endianness = CWAV_ENDIANNESS_LITTLE;
    header->headerSize = (u16) headerSize;
    header->version = CWAV_VERSION;
    header->fileSize = outputSize;
    header->numBlocks = 2;
    header->infoBlock.ref.typeId = CWAV_REF_INFO_BLOCK;
    header->infoBlock.ref.offset = headerSize;
    header->infoBlock.size = infoSize;
    header->dataBlock.ref.typeId = CWAV_REF_DATA_BLOCK;
    header->dataBlock.ref.offset = headerSize + infoSize;
    header->dataBlock.size = dataSize;

    CWAVInfoBlockHeader* infoBlockHeader = (CWAVInfoBlockHeader*) &((u8*) output)[headerSize];
    memcpy(infoBlockHeader->header.magic, CWAV_BLOCK_MAGIC_INFO, sizeof(infoBlockHeader->header.magic));
    infoBlockHeader->header.size = infoSize;
    infoBlockHeader->encoding = cwav->encoding;
    infoBlockHeader->loop = cwav->loop;
    infoBlockHeader->sampleRate = cwav->sampleRate;
    infoBlockHeader->loopStartFrame = cwav->loopStartFrame;
    infoBlockHeader->loopEndFrame = cwav->loopEndFrame;
    infoBlockHeader->channelInfos.count = cwav->channels;
    for(u32 c = 0; c < cwav->channels; c++) {
        infoBlockHeader->channelInfos.contents[c].typeId = CWAV_REF_CHANNEL_INFO;
        infoBlockHeader->channelInfos.contents[c].offset = sizeof(CWAVReferenceTable) + (cwav->channels * sizeof(CWAVReference)) + (c * sizeof(CWAVChannelInfo));

        CWAVChannelInfo* info = (CWAVChannelInfo*) &((u8*) output)[headerSize + sizeof(CWAVInfoBlockHeader) + (cwav->channels * sizeof(CWAVReference)) + (c * sizeof(CWAVChannelInfo))];
        info->samples.typeId = CWAV_REF_SAMPLE_DATA;
        info->samples.offset = (((sizeof(CWAVDataBlock) + 0x1F) & ~0x1F) - sizeof(CWAVDataBlock)) + (c * (cwav->dataSize / cwav->channels));
        if (cwav->encoding == CWAV_ENCODING_PCM16 || cwav->encoding == CWAV_ENCODING_PCM8) {
            info->adpcmInfo.typeId = 0;
            info->adpcmInfo.offset = 0xFFFFFFFF;
        } else if (cwav->encoding == CWAV_ENCODING_IMA_ADPCM) {
            info->adpcmInfo.typeId = CWAV_REF_IMA_ADPCM_INFO;
            CWAVIMAADPCMInfo* imainfo = (CWAVIMAADPCMInfo*)((u8*)output + adpcmOffset + c * sizeof(CWAVIMAADPCMInfo));
            info->adpcmInfo.offset = (u8*)imainfo - (u8*)&info->samples;
            imainfo->context.data = cwav->imainfos[c].sample;
            imainfo->context.tableIndex = cwav->imainfos[c].index;
            imainfo->loopContext.data = cwav->imainfosloop[c].sample;
            imainfo->loopContext.tableIndex = cwav->imainfosloop[c].index;
        } else if (cwav->encoding == CWAV_ENCODING_DSP_ADPCM) {
            info->adpcmInfo.typeId = CWAV_REF_DSP_ADPCM_INFO;
            CWAVDSPADPCMInfo* dspinfo = (CWAVDSPADPCMInfo*)((u8*)output + adpcmOffset + c * sizeof(CWAVDSPADPCMInfo));
            info->adpcmInfo.offset = (u8*)dspinfo - (u8*)&info->samples;
            memcpy(dspinfo->param.coefficients, cwav->dspinfos[c].coefs, sizeof(u16) * 16);
            dspinfo->context.predictorScale = cwav->dspinfos[c].info.index;
            dspinfo->context.previousSample = cwav->dspinfos[c].info.hist1;
            dspinfo->context.secondPreviousSample = cwav->dspinfos[c].info.hist2;
            dspinfo->loopContext.predictorScale = cwav->dspinfos[c].loopinfo.index;
            dspinfo->loopContext.previousSample = cwav->dspinfos[c].loopinfo.hist1;
            dspinfo->loopContext.secondPreviousSample = cwav->dspinfos[c].loopinfo.hist2;
        }
    }

    CWAVDataBlock* dataBlock = (CWAVDataBlock*) &((u8*) output)[headerSize + infoSize];
    memcpy(dataBlock->header.magic, CWAV_BLOCK_MAGIC_DATA, sizeof(dataBlock->header.magic));
    dataBlock->header.size = dataSize;

    memcpy((u8*)output + ((headerSize + infoSize + sizeof(CWAVBlockHeader) + 0x1F) & ~0x1F), cwav->data, cwav->dataSize);

    if(size != NULL) {
        *size = outputSize;
    }

    return output;
}