#ifndef GROK_H_
#define GROK_H_

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
    struct adpcminfo
    {
        uint8_t index;
        int16_t hist1;
        int16_t hist2;
    };
    int16_t coefs[16];
    struct adpcminfo info;
    struct adpcminfo loopinfo;    
} DSPADPCMInfo;

#ifdef __cplusplus
extern "C" {
#endif
void DSPCorrelateCoefs(const short* source, int samples, short* coefsOut);
void DSPEncodeFrame(short pcmInOut[16], int sampleCount, unsigned char adpcmOut[8], const short coefsIn[8][2]);
#ifdef __cplusplus
}
#endif

#endif