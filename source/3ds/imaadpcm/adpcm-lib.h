////////////////////////////////////////////////////////////////////////////
//                           **** ADPCM-XQ ****                           //
//                  Xtreme Quality ADPCM Encoder/Decoder                  //
//                    Copyright (c) 2015 David Bryant.                    //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

#ifndef ADPCMLIB_H_
#define ADPCMLIB_H_

#if defined(_MSC_VER) && _MSC_VER < 1600
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;
typedef __int64 int64_t;
typedef __int32 int32_t;
typedef __int16 int16_t;
typedef __int8  int8_t;
#else
#include <stdint.h>
#endif

typedef struct
{
    uint16_t sample;
    uint8_t index;
} IMAADPCMInfo;

#ifdef __cplusplus
extern "C" {
#endif

void adpcm_calculate_initial_deltas(int32_t initial_deltas[2], int num_channels, const int16_t *inbuf, int inbufcount);
void *adpcm_create_context (int num_channels, int lookahead, int noise_shaping, int32_t initial_deltas [2]);
int adpcm_encode_block (void *p, IMAADPCMInfo* infos, uint8_t *outbuf, size_t *outbufsize, const int16_t *inbuf, int inbufcount);
void adpcm_free_context (void *p);

#ifdef __cplusplus
}
#endif

#define NOISE_SHAPING_OFF       0   // flat noise (no shaping)
#define NOISE_SHAPING_STATIC    1   // first-order highpass shaping
#define NOISE_SHAPING_DYNAMIC   2   // dynamically tilted noise based on signal

#endif /* ADPCMLIB_H_ */
