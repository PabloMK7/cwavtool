#include "wav.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

bool find_chunk(FILE* fd, const char* magic) {
    char curr[5] = {0};
    while(strcmp(curr, magic) != 0) {
        u32 read = (u32) fread(curr, 1, 4, fd);
        if(read == 0) {
            return false;
        }
    }

    fseek(fd, -4, SEEK_CUR);
    return true;
}

WAV* read_wav(const char* file) {
    FILE* fd = fopen(file, "r");
    if(!fd) {
        printf("ERROR: Could not open WAV file: %s\n", strerror(errno));
        return NULL;
    }

    if(!find_chunk(fd, "RIFF")) {
        printf("ERROR: Could not find WAV RIFF chunk.\n");
        return NULL;
    }

    Riff riff;
    fread(&riff, sizeof(Riff), 1, fd);

    if(!find_chunk(fd, "fmt ")) {
        printf("ERROR: Could not find WAV format chunk.\n");
        return NULL;
    }

    Format format;
    fread(&format, sizeof(Format), 1, fd);

    if(!find_chunk(fd, "data")) {
        printf("ERROR: Could not find WAV data chunk.\n");
        return NULL;
    }

    Data data;
    fread(&data, sizeof(Data), 1, fd);

    u8* dataBytes = (u8*) malloc(data.chunkSize);
    fread(dataBytes, 1, data.chunkSize, fd);

    fclose(fd);

    WAV* wav = (WAV*) malloc(sizeof(WAV));
    wav->riff = riff;
    wav->format = format;
    wav->data = data;
    wav->dataBytes = dataBytes;
    return wav;
}