#include "cmd.h"

#include "3ds/cwav.h"
#include "pc/stb_vorbis.h"
#include "pc/wav.h"

#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <codecvt>
#include <locale>
#include <map>
#include <vector>

static void* read_file(u32* size, const std::string& file) {
    FILE* fd = fopen(file.c_str(), "rb");
    if(fd == NULL) {
        perror("ERROR: Could not open file for reading");
        return NULL;
    }

    long tell = 0;
    if(fseek(fd, 0, SEEK_END) != 0 || (tell = ftell(fd)) < 0 || fseek(fd, 0, SEEK_SET) != 0) {
        fclose(fd);

        perror("ERROR: Failed to determine file size");
        return NULL;
    }

    size_t bufferSize = (size_t) tell;
    void* buffer = malloc(bufferSize);
    if(buffer == NULL) {
        fclose(fd);

        printf("ERROR: Could not allocate memory for file data.\n");
        return NULL;
    }

    size_t readRet = fread(buffer, 1, bufferSize, fd);

    fclose(fd);

    if(readRet != bufferSize) {
        free(buffer);

        perror("ERROR: Failed to read file");
        return NULL;
    }

    if(size != NULL) {
        *size = bufferSize;
    }

    return buffer;
}

static bool write_file(void* contents, u32 size, const std::string& file) {
    FILE* fd = fopen(file.c_str(), "wb");
    if(fd == NULL) {
        perror("ERROR: Could not open file for writing");
        return false;
    }

    size_t writeRet = fwrite(contents, 1, size, fd);

    fclose(fd);

    if(writeRet != size) {
        perror("ERROR: Failed to write file");
        return false;
    }

    return true;
}

static void* convert_to_cwav(u32* size, const std::string& file, u32 encoding, bool loop, u32 loopStartFrame, u32 loopEndFrame) {
    FILE* fd = fopen(file.c_str(), "rb");
    if(fd == NULL) {
        perror("ERROR: Failed to open file for CWAV conversion");
        return NULL;
    }

    char magic[4];
    if(fread(magic, 1, sizeof(magic), fd) != sizeof(magic)) {
        fclose(fd);

        perror("ERROR: Failed to read audio file magic");
        return NULL;
    }

    rewind(fd);

    CWAV cwav;
    memset(&cwav, 0, sizeof(cwav));

    cwav.encoding = encoding;
    cwav.imainfos = NULL;
    cwav.imainfosloop = NULL;
    cwav.dspinfos = NULL;
    cwav.loop = loop;
    cwav.loopStartFrame = loopStartFrame;
    cwav.loopEndFrame = loopEndFrame;

    if(memcmp(magic, "RIFF", sizeof(magic)) == 0) {
        WAV* wav = wav_read(fd);
        if(wav != NULL) {
            cwav.channels = wav->format.numChannels;
            cwav.sampleRate = wav->format.sampleRate;
            cwav.bitsPerSample = wav->format.bitsPerSample;

            cwav.dataSize = wav->data.size;
            cwav.data = calloc(wav->data.size, sizeof(u8));
            if (loopEndFrame == 0 || loopEndFrame > cwav.dataSize / cwav.channels / (cwav.bitsPerSample / 8)) {
                cwav.loopEndFrame = cwav.dataSize / cwav.channels / (cwav.bitsPerSample / 8);
            }
            if(cwav.data != NULL) {
                memcpy(cwav.data, wav->data.data, wav->data.size);
            } else {
                printf("ERROR: Could not allocate memory for CWAV sample data.\n");
            }

            wav_free(wav);
        }
    } else if(memcmp(magic, "OggS", sizeof(magic)) == 0) {
        int error = 0;
        stb_vorbis* vorbis = stb_vorbis_open_file(fd, false, &error, NULL);

        if(vorbis != NULL) {
            stb_vorbis_info info = stb_vorbis_get_info(vorbis);
            u32 sampleCount = stb_vorbis_stream_length_in_samples(vorbis) * info.channels;

            cwav.channels = (u32) info.channels;
            cwav.sampleRate = info.sample_rate;
            cwav.bitsPerSample = sizeof(u16) * 8;

            cwav.dataSize = sampleCount * sizeof(u16);
            if (loopEndFrame == 0 || loopEndFrame > sampleCount / info.channels) {
                cwav.loopEndFrame = sampleCount / info.channels;
            }
            cwav.data = calloc(sampleCount, sizeof(u16));
            if(cwav.data != NULL) {
                stb_vorbis_get_samples_short_interleaved(vorbis, info.channels, (short*) cwav.data, sampleCount);
            } else {
                printf("ERROR: Could not allocate memory for CWAV sample data.\n");
            }

            stb_vorbis_close(vorbis);
        } else {
            printf("ERROR: Failed to open vorbis file: %d.\n", error);
        }
    } else {
        printf("ERROR: Audio file magic '%c%c%c%c' unrecognized.\n", magic[0], magic[1], magic[2], magic[3]);
    }

    fclose(fd);

    if(cwav.data == NULL) {
        return NULL;
    }

    if (cwav.loopStartFrame >= cwav.loopEndFrame) {
        printf("ERROR: Invalid loop range.\n");
        free(cwav.data);
        return NULL;
    }

    if (encoding == CWAV_ENCODING_IMA_ADPCM) {
        cwav.imainfos = (IMAADPCMInfo*)malloc(cwav.channels * sizeof(IMAADPCMInfo));
        cwav.imainfosloop = (IMAADPCMInfo*)malloc(cwav.channels * sizeof(IMAADPCMInfo));
    } else if (encoding == CWAV_ENCODING_DSP_ADPCM) {
        cwav.dspinfos = (DSPADPCMInfo*)malloc(cwav.channels * sizeof(DSPADPCMInfo));
    }

    void* ret = cwav_build(size, &cwav);

    free(cwav.data);
    if (cwav.imainfos) free(cwav.imainfos);
    if (cwav.imainfosloop) free(cwav.imainfosloop);
    if (cwav.dspinfos) free(cwav.dspinfos);
    return ret;
}


static int cmd_make_cwav(const std::string& input, const std::string& output, u32 encoding, bool loop, u32 loopStartFrame, u32 loopEndFrame) {
    u32 cwavSize = 0;
    void* cwav = convert_to_cwav(&cwavSize, input, encoding, loop, loopStartFrame, loopEndFrame);
    if(cwav == NULL || !write_file(cwav, cwavSize, output)) {
        return 1;
    }

    printf("Created CWAV \"%s\".\n", output.c_str());
    return 0;
}


static std::map<std::string, std::string> cmd_get_args(int argc, char* argv[]) {
    std::map<std::string, std::string> args;
    for(int i = 0; i < argc; i++) {
        if((strncmp(argv[i], "-", 1) == 0 || strncmp(argv[i], "--", 2) == 0) && argc != i + 1) {
            args[argv[i]] = argv[i + 1];
            i++;
        }
    }

    return args;
}

static std::string cmd_find_arg(const std::map<std::string, std::string>& args, const std::string& shortOpt, const std::string& longOpt, const std::string& def) {
    std::string sopt = "-" + shortOpt;
    std::string lopt = "--" + longOpt;

    std::map<std::string, std::string>::const_iterator match = args.find(sopt);
    if(match != args.end()) {
        return (*match).second;
    }

    match = args.find(lopt);
    if(match != args.end()) {
        return (*match).second;
    }

    return def;
}

static std::vector<std::string> cmd_parse_list(const std::string& list) {
    std::vector<std::string> ret;
    std::string::size_type lastPos = 0;
    std::string::size_type pos = 0;
    while((pos = list.find(',', lastPos)) != std::string::npos) {
        ret.push_back(list.substr(lastPos, pos - lastPos));
        lastPos = pos + 1;
    }

    if(lastPos < list.size()) {
        ret.push_back(list.substr(lastPos));
    }

    return ret;
}

static void cmd_print_commands() {
    printf("Available arguments:\n");
    printf("  -i/--input: WAV/OGG input file.\n");
    printf("  -o/--output: CWAV output file.\n");
    printf("  -e/--encoding: Optional. Encoding of the created CWAV (pcm8/pcm16/imaadpcm/dspadpcm).\n");
    printf("  -ls/--loopstartframe: Optional. Sample to return to when looping.\n");
    printf("  -le/--loopendframe: Optional. Sample to loop at or \"end\".\n");
}

static void cmd_print_usage(const std::string& executedFrom) {
    printf("cwavtool v%d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
    printf("Usage: %s <args>\n", executedFrom.c_str());
    cmd_print_commands();
}

static void cmd_missing_args() {
    printf("Missing arguments.\n");
    cmd_print_commands();
}

static void cmd_invalid_arg(const std::string& argument) {
    printf("Invalid value for argument \"%s\".\n", argument.c_str());
    cmd_print_commands();
}

static void cmd_invalid_arg_combination() {
    printf("Invalid combination of arguments.\n");
    cmd_print_commands();
}

int cmd_process_command(int argc, char* argv[]) {
    if(argc <= 1) {
        cmd_print_usage(argv[0]);
        return -1;
    }

    std::map<std::string, std::string> args = cmd_get_args(argc, argv);

    const std::string input = cmd_find_arg(args, "i", "input", "");
    const std::string output = cmd_find_arg(args, "o", "output", "");
    std::string encoding = cmd_find_arg(args, "e", "encoding", "pcm16");
    int loopStartFrame = atoi(cmd_find_arg(args, "ls", "loopstartframe", "-1").c_str());
    std::string loopEndStr = cmd_find_arg(args, "le", "loopendframe", "-1");
    std::transform(loopEndStr.begin(), loopEndStr.end(), loopEndStr.begin(), (int (*)(int)) std::tolower);
    int loopEndFrame;
    if (loopEndStr == "end")
        loopEndFrame = INT32_MAX;
    else
        loopEndFrame = atoi(loopEndStr.c_str());
    if(input.empty() || output.empty()) {
        cmd_missing_args();
        return -1;
    }
    bool loop = false;

    if (loopStartFrame < 0 && loopEndFrame < 0) {
        loop = false;
        loopStartFrame = 0;
        loopEndFrame = 0;
    } else if (loopStartFrame >= 0 && loopEndFrame >= 0)
        loop = true;
    else {
        cmd_invalid_arg_combination();
        return -1;
    }

    if (loop && loopStartFrame >= loopEndFrame) {
        printf("ERROR: Invalid loop range.\n");
        return -1;
    }

    u32 audioenc;
    std::transform(encoding.begin(), encoding.end(), encoding.begin(), (int (*)(int)) std::tolower);
    if (encoding == "pcm16") audioenc = CWAV_ENCODING_PCM16;
    else if (encoding == "pcm8") audioenc = CWAV_ENCODING_PCM8;
    else if (encoding == "imaadpcm") audioenc = CWAV_ENCODING_IMA_ADPCM;
    else if (encoding == "dspadpcm") audioenc = CWAV_ENCODING_DSP_ADPCM;
    else {cmd_invalid_arg("encoding"); return -1;}

    return cmd_make_cwav(input, output, audioenc, loop, loopStartFrame, loopEndFrame);
}
