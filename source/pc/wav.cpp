#include "wav.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

bool wav_find_chunk(FILE* fd, const char* magic) {
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

WAV* wav_read(FILE* fd) {
    if(!fd) {
        printf("ERROR: Could not open WAV file: %s\n", strerror(errno));
        return NULL;
    }

    if(!wav_find_chunk(fd, "RIFF")) {
        printf("ERROR: Could not find WAV RIFF chunk.\n");
        return NULL;
    }

    Riff riff;
    fread(&riff, sizeof(Riff), 1, fd);

    if(!wav_find_chunk(fd, "fmt ")) {
        printf("ERROR: Could not find WAV format chunk.\n");
        return NULL;
    }

    Format format;
    fread(&format, sizeof(Format), 1, fd);

    if(!wav_find_chunk(fd, "data")) {
        printf("ERROR: Could not find WAV data chunk.\n");
        return NULL;
    }

    Data data;
    fread(&(data.chunkId), sizeof(data.chunkId), 1, fd);
    fread(&(data.chunkSize), sizeof(data.chunkSize), 1, fd);
    data.data = (u8*) malloc(data.chunkSize);
    fread(data.data, 1, data.chunkSize, fd);

    WAV* wav = (WAV*) malloc(sizeof(WAV));
    wav->riff = riff;
    wav->format = format;
    wav->data = data;
    return wav;
}

void wav_free(WAV* wav) {
    if(wav != NULL) {
        free(wav->data.data);
        free(wav);
    }
}