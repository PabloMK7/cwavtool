#ifndef __CBMD_H__
#define __CBMD_H__

#include "types.h"

typedef enum {
    CGFX_COMMON,
    CGFX_EUR_ENGLISH,
    CGFX_EUR_FRENCH,
    CGFX_EUR_GERMAN,
    CGFX_EUR_ITALIAN,
    CGFX_EUR_SPANISH,
    CGFX_EUR_DUTCH,
    CGFX_EUR_PORTUGESE,
    CGFX_EUR_RUSSIAN,
    CGFX_JPN_JAPANESE,
    CGFX_USA_ENGLISH,
    CGFX_USA_FRENCH,
    CGFX_USA_SPANISH,
    CGFX_USA_PORTUGESE
} CBMDCGFX;

typedef struct {
    u8* cgfxs[14] = {NULL};
    u32 cgfxSizes[14] = {0};
    u8* cwav = NULL;
    u32 cwavSize = 0;
} CBMD;

u8* build_cbmd(CBMD cbmd, u32* size);
u8* build_bnr(CBMD cbmd, u32* size);

#endif