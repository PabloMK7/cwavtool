#include "wav.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct {
    FILE* fd;
    WavChunkHeader currChunk;
    long currChunkDataPos;
} WavContext;

static bool wav_next_chunk(WavContext* context) {
    if(fseek(context->fd, context->currChunkDataPos + (memcmp(context->currChunk.chunkId, "RIFF", 4) == 0 ? 4 : context->currChunk.chunkSize), SEEK_SET) < 0) {
        return false;
    }

    if(fread(&context->currChunk, sizeof(WavChunkHeader), 1, context->fd) <= 0) {
        return false;
    }

    context->currChunkDataPos = ftell(context->fd);
    return true;
}

static bool wav_read_chunk_data(WavContext* context, void* data, size_t maxSize) {
    if(fseek(context->fd, context->currChunkDataPos, SEEK_SET) < 0) {
        return false;
    }

    size_t size = context->currChunk.chunkSize < maxSize ? context->currChunk.chunkSize : maxSize;
    if(fread(data, size, 1, context->fd) <= 0) {
        return false;
    }

    return true;
}

WAV* wav_read(FILE* fd) {
    if(!fd) {
        printf("ERROR: Could not open WAV file: %s\n", strerror(errno));
        return NULL;
    }

    WAV* wav = (WAV*) calloc(1, sizeof(WAV));

    WavContext context;
    memset(&context, 0, sizeof(context));
    context.fd = fd;

    u32 foundChunks = 0;
    while(wav_next_chunk(&context)) {
        if(memcmp(context.currChunk.chunkId, "RIFF", 4) == 0) {
            if(wav_read_chunk_data(&context, &wav->riff, sizeof(WavRiffChunk))) {
                foundChunks++;
            }
        } else if(memcmp(context.currChunk.chunkId, "fmt ", 4) == 0) {
            if(wav_read_chunk_data(&context, &wav->format, sizeof(WavFormatChunk))) {
                foundChunks++;
            }
        } else if(memcmp(context.currChunk.chunkId, "data", 4) == 0) {
            wav->data.size = context.currChunk.chunkSize;
            wav->data.data = (u8*) malloc(wav->data.size);
            if(wav_read_chunk_data(&context, wav->data.data, wav->data.size)) {
                foundChunks++;
            }
        }
    }

    if(foundChunks != 3) {
        wav_free(wav);

        printf("ERROR: Failed to read WAV chunks: %s\n", errno != 0 ? strerror(errno) : "Not enough chunks.");
        return NULL;
    }

    return wav;
}

void wav_free(WAV* wav) {
    if(wav != NULL) {
        if(wav->data.data != NULL) {
            free(wav->data.data);
            wav->data.data = NULL;
        }

        free(wav);
    }
}
