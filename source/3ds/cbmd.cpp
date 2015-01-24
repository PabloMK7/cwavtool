#include "cbmd.h"

#include "lz11.h"

#include <string.h>
#include <malloc.h>

typedef struct {
    char magic[4] = {'C', 'B', 'M', 'D'};
    u32 zero = 0;
    u32 cgfxOffsets[14] = {0};
    u8 padding[0x44] = {0};
    u32 cwavOffset = 0;
} CBMDHeader;

u8* build_cbmd_data(CBMD cbmd, u32* size, bool bnr) {
    u32 headerSize = sizeof(CBMDHeader);
    CBMDHeader header;

    u8* compressedCGFXs[14] = {0};
    u32 compressedCGFXSizes[14] = {0};

    u32 offset = headerSize;
    for(int i = 0; i < 14; i++) {
        if(cbmd.cgfxs[i] != NULL) {
            compressedCGFXs[i] = lz11_compress(cbmd.cgfxs[i], cbmd.cgfxSizes[i], &compressedCGFXSizes[i]);
            header.cgfxOffsets[i] = offset;
            offset += compressedCGFXSizes[i];
        }
    }

    u32 pad = 0;
    if(bnr) {
        pad = 16 - (offset % 16);
        offset += pad;
    }

    if(cbmd.cwav != NULL) {
        header.cwavOffset = offset;
        offset += cbmd.cwavSize;
    }

    u8* output = (u8*) malloc(offset);
    u32 pos = 0;

    memcpy(output + pos, &header, headerSize);
    pos += headerSize;

    for(int i = 0; i < 14; i++) {
        if(compressedCGFXs[i] != NULL) {
            memcpy(output + pos, compressedCGFXs[i], compressedCGFXSizes[i]);
            free(compressedCGFXs[i]);
            compressedCGFXs[i] = NULL;
            pos += compressedCGFXSizes[i];
        }
    }

    if(bnr) {
        memset(output + pos, 0, pad);
        pos += pad;
    }

    if(cbmd.cwav != NULL) {
        memcpy(output + pos, cbmd.cwav, cbmd.cwavSize);
        pos += cbmd.cwavSize;
    }

    if(size != NULL) {
        *size = offset;
    }

    return output;
}

u8* build_cbmd(CBMD cbmd, u32* size) {
    return build_cbmd_data(cbmd, size, false);
}

u8* build_bnr(CBMD cbmd, u32* size) {
    return build_cbmd_data(cbmd, size, true);
}