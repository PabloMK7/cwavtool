#include "cmd.h"

#include "3ds/3ds.h"
#include "pc/wav.h"
#include "pc/stb_vorbis.h"
#include "types.h"
#include "3ds/smdh.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <string>
#include <vector>

u8* convert_to_cgfx(const std::string& image, u32 width, u32 height, u32* size) {
    u32 convertedSize = 0;
    u16* converted = image_to_tiles(image.c_str(), width, height, RGBA4444, &convertedSize);
    if(converted == NULL) {
        return NULL;
    }

    u8* ret = (u8*) malloc(BANNER_CGFX_HEADER_LENGTH + convertedSize);
    memcpy(ret, BANNER_CGFX_HEADER, BANNER_CGFX_HEADER_LENGTH);
    memcpy(ret + BANNER_CGFX_HEADER_LENGTH, converted, convertedSize);

    *size = BANNER_CGFX_HEADER_LENGTH + convertedSize;
    free(converted);
    return ret;
}

u8* convert_to_cwav(const std::string& file, u32* size) {
    u8* ret = NULL;
    // Determine what file type we have
    FILE* fd = fopen(file.c_str(), "rb");
    char magic[4];
    fread(magic, sizeof(magic), 1, fd);
    rewind(fd); // equivalent to SEEK_SET to pos 0

    if (magic[0] == 'R' && magic[1] == 'I' && magic[2] == 'F' && magic[3] == 'F') {
        WAV* wav = wav_read(fd);
        if(wav != NULL) {
            CWAV cwav;
            cwav.channels = wav->format.numChannels;
            cwav.sampleRate = wav->format.sampleRate;
            cwav.bitsPerSample = wav->format.bitsPerSample;
            cwav.dataSize = wav->data.size;
            cwav.data = wav->data.data;

            ret = cwav_build(cwav, size);

            wav_free(wav);
        }
    } else if (magic[0] == 'O' && magic[1] == 'g' && magic[2] == 'g' && magic[3] == 'S') {
        int error;
        stb_vorbis* vorb = stb_vorbis_open_file(fd, false, &error, NULL);
        if(vorb != NULL) {
            stb_vorbis_info info = stb_vorbis_get_info(vorb);

            CWAV cwav;
            cwav.channels = (u32) info.channels;
            cwav.sampleRate = info.sample_rate;
            cwav.bitsPerSample = 16; // stb_vorbis always outputs 16 bit samples
            u32 sampleCount = stb_vorbis_stream_length_in_samples(vorb) * info.channels;
            cwav.dataSize = sampleCount * 2;
            cwav.data = (u8*) calloc(sampleCount, 2);
            stb_vorbis_get_samples_short_interleaved(vorb, info.channels, (short*) cwav.data, sampleCount);

            ret = cwav_build(cwav, size);

            free(cwav.data);
            stb_vorbis_close(vorb);
        } else {
            printf("ERROR: Vorbis open failed, error %d.\n", error);
        }
    } else {
        printf("ERROR: Audio file header '%c%c%c%c' unrecognized.\n", magic[0], magic[1], magic[2], magic[3]);
    }

    fclose(fd);
    return ret;
}

int cmd_make_banner(const std::string& image, const std::string& audio, const std::string& cgfxFile, const std::string& cwavFile, const std::string& output) {
    u32 cgfxSize = 0;
    u8* cgfx = NULL;
    if(!cgfxFile.empty()) {
        FILE* fd = fopen(cgfxFile.c_str(), "r");
        if(!fd) {
            printf("ERROR: Could not open CGFX file: %s\n", strerror(errno));
            return 1;
        }

        fseek(fd, 0, SEEK_END);
        cgfxSize = (u32) ftell(fd);
        fseek(fd, 0, SEEK_SET);

        cgfx = (u8*) malloc(cgfxSize);
        fread(cgfx, 1, cgfxSize, fd);
        fclose(fd);
    } else {
        cgfx = convert_to_cgfx(image, 256, 128, &cgfxSize);
        if(cgfx == NULL) {
            return 2;
        }
    }

    u32 cwavSize = 0;
    u8* cwav = NULL;
    if(!cwavFile.empty()) {
        FILE* fd = fopen(cwavFile.c_str(), "r");
        if(!fd) {
            printf("ERROR: Could not open CWAV file: %s\n", strerror(errno));
            return 3;
        }

        fseek(fd, 0, SEEK_END);
        cwavSize = (u32) ftell(fd);
        fseek(fd, 0, SEEK_SET);

        cwav = (u8*) malloc(cwavSize);
        fread(cwav, 1, cwavSize, fd);
        fclose(fd);
    } else {
        cwav = convert_to_cwav(audio, &cwavSize);
        if(cwav == NULL) {
            return 4;
        }
    }

    CBMD cbmd;
    cbmd.cgfxs[CGFX_COMMON] = cgfx;
    cbmd.cgfxSizes[CGFX_COMMON] = cgfxSize;
    cbmd.cwav = cwav;
    cbmd.cwavSize = cwavSize;

    u32 bnrSize = 0;
    u8* bnr = bnr_build(cbmd, &bnrSize);
    free(cgfx);
    free(cwav);

    FILE* fd = fopen(output.c_str(), "wb");
    if(fd == NULL) {
        printf("ERROR: Could not open output file: %s\n", strerror(errno));
        return 5;
    }

    fwrite(bnr, 1, bnrSize, fd);
    fclose(fd);

    free(bnr);

    printf("Created banner \"%s\".\n", output.c_str());
    return 0;
}

int cmd_make_smdh(const std::string& shortTitle, const std::string& longTitle, const std::string& publisher, const std::string& icon, SMDH base, const std::string& output) {
    u8* icon48Data = load_image(icon.c_str(), 48, 48);
    if(icon48Data == NULL) {
        return 1;
    }

    u16* icon48 = image_data_to_tiles(icon48Data, 48, 48, RGB565, NULL);
    if(icon48 == NULL) {
        return 1;
    }

    u8 icon24Data[24 * 24 * 4];
    for(int y = 0; y < 48; y += 2) {
        for(int x = 0; x < 48; x += 2) {
            int i1 = (y * 48 + x) * 4;
            u8 r1 = icon48Data[i1 + 0];
            u8 g1 = icon48Data[i1 + 1];
            u8 b1 = icon48Data[i1 + 2];
            u8 a1 = icon48Data[i1 + 3];

            int i2 = (y * 48 + (x + 1)) * 4;
            u8 r2 = icon48Data[i2 + 0];
            u8 g2 = icon48Data[i2 + 1];
            u8 b2 = icon48Data[i2 + 2];
            u8 a2 = icon48Data[i2 + 3];

            int i3 = ((y + 1) * 48 + x) * 4;
            u8 r3 = icon48Data[i3 + 0];
            u8 g3 = icon48Data[i3 + 1];
            u8 b3 = icon48Data[i3 + 2];
            u8 a3 = icon48Data[i3 + 3];

            int i4 = ((y + 1) * 48 + (x + 1)) * 4;
            u8 r4 = icon48Data[i4 + 0];
            u8 g4 = icon48Data[i4 + 1];
            u8 b4 = icon48Data[i4 + 2];
            u8 a4 = icon48Data[i4 + 3];

            int id = ((y / 2) * 24 + (x / 2)) * 4;
            icon24Data[id + 0] = (u8) ((r1 + r2 + r3 + r4) / 4);
            icon24Data[id + 1] = (u8) ((g1 + g2 + g3 + g4) / 4);
            icon24Data[id + 2] = (u8) ((b1 + b2 + b3 + b4) / 4);
            icon24Data[id + 3] = (u8) ((a1 + a2 + a3 + a4) / 4);
        }
    }

    u16* icon24 = image_data_to_tiles(icon24Data, 24, 24, RGB565, NULL);
    if(icon24 == NULL) {
        return 1;
    }

    for(int i = 0; i < 0x10; i++) {
        utf8_to_utf16(base.titles[i].shortTitle, shortTitle.c_str(), 0x40);
        utf8_to_utf16(base.titles[i].longTitle, longTitle.c_str(), 0x80);
        utf8_to_utf16(base.titles[i].publisher, publisher.c_str(), 0x40);
    }

    memcpy(base.largeIcon, icon48, 0x1200);
    memcpy(base.smallIcon, icon24, 0x480);
    free(icon48);

    FILE* fd = fopen(output.c_str(), "wb");
    if(fd == NULL) {
        printf("ERROR: Could not open output file: %s\n", strerror(errno));
        return 2;
    }

    fwrite(&base, 1, sizeof(SMDH), fd);
    fclose(fd);

    printf("Created SMDH \"%s\".\n", output.c_str());
    return 0;
}

int cmd_make_cwav(const std::string& input, const std::string& output) {
    u32 cwavSize = 0;
    u8* cwav = convert_to_cwav(input, &cwavSize);
    if(cwav == NULL) {
        return 1;
    }

    FILE* fd = fopen(output.c_str(), "wb");
    if(fd == NULL) {
        printf("ERROR: Could not open output file: %s\n", strerror(errno));
        return 2;
    }

    fwrite(cwav, 1, cwavSize, fd);
    fclose(fd);

    free(cwav);

    printf("Created CWAV \"%s\".\n", output.c_str());
    return 0;
}

int cmd_lz11(const std::string& input, const std::string& output) {
    FILE* in = fopen(input.c_str(), "r");
    if(in == NULL) {
        printf("ERROR: Could not open input file: %s\n", strerror(errno));
        return 1;
    }

    fseek(in, 0, SEEK_END);
    u32 size = (u32) ftell(in);
    fseek(in, 0, SEEK_SET);

    u8* data = (u8*) malloc(size);
    if(data == NULL) {
        printf("ERROR: Could not allocate data buffer.\n");
        return 2;
    }

    fread(data, 1, size, in);
    fclose(in);

    u32 compressedSize;
    u8* compressed = lz11_compress(data, size, &compressedSize);
    free(data);
    if(compressed == NULL) {
        return 3;
    }

    FILE* fd = fopen(output.c_str(), "wb");
    if(fd == NULL) {
        printf("ERROR: Could not open output file: %s\n", strerror(errno));
        return 4;
    }

    fwrite(compressed, 1, compressedSize, fd);
    fclose(fd);

    free(compressed);

    printf("Compressed to file \"%s\".\n", output.c_str());
    return 0;
}

std::map<std::string, std::string> cmd_get_args(int argc, char* argv[]) {
    std::map<std::string, std::string> args;
    for(int i = 0; i < argc; i++) {
        if((strncmp(argv[i], "-", 1) == 0 || strncmp(argv[i], "--", 2) == 0) && argc != i + 1) {
            args.insert(std::pair<std::string, std::string>(argv[i], argv[i + 1]));
            i++;
        }
    }

    return args;
}

std::string cmd_find_arg(const std::map<std::string, std::string>& args, const std::string& shortOpt, const std::string& longOpt, const std::string& def) {
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

std::vector<std::string> cmd_parse_list(const std::string& list) {
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

void cmd_print_info(const std::string& command) {
    if(command.compare("makebanner") == 0) {
        printf("makebanner - Creates a .bnr file.\n");
        printf("  -i/--image: PNG file to use as the banner's image. Interchangeable with -ci.\n");
        printf("  -a/--audio: WAV file to use as the banner's tune. Interchangeable with -ca.\n");
        printf("  -ci/--cgfximage: CGFX file to use as the banner's image. Interchangeable with -i.\n");
        printf("  -ca/--cwavaudio: CWAV file to use as the banner's tune. Interchangeable with -a.\n");
        printf("  -o/--output: File to output the created banner to.\n");
    } else if(command.compare("makesmdh") == 0) {
        printf("makesmdh - Creates a .smdh/.icn file.\n");
        printf("  -s/--shorttitle: Short title of the application.\n");
        printf("  -l/--longtitle: Long title of the application.\n");
        printf("  -p/--publisher: Publisher of the application.\n");
        printf("  -i/--icon: PNG file to use as an icon.\n");
        printf("  -o/--output: File to output the created SMDH/ICN to.\n");
        printf("  -r/--regions: Optional. Comma separated list of regions to lock the SMDH to.\n");
        printf("     Valid regions: regionfree, japan, northamerica, europe, australia, china, korea, taiwan.\n");
        printf("  -f/--flags: Optional. Flags to apply to the SMDH file.\n");
        printf("     Valid flags: visible, autoboot, allow3d, requireeula, autosave, extendedbanner, ratingrequired, savedata, recordusage, nosavebackups, new3ds.\n");
        printf("  -mmid/--matchmakerid: Optional. Match maker ID of the SMDH.\n");
        printf("  -ev/--eulaversion: Optional. Version of the EULA required to be accepted before launching.\n");
        printf("  -obf/--optimalbannerframe: Optional. Optimal frame of the accompanying banner.\n");
        printf("  -spid/--streetpassid: Optional. Streetpass ID of the SMDH.\n");
        printf("  -cer/--cero: Optional. CERO rating number (0-255).\n");
        printf("  -er/--esrb: Optional. ESRB rating number (0-255).\n");
        printf("  -ur/--usk: Optional. USK rating number (0-255).\n");
        printf("  -pgr/--pegigen: Optional. PEGI GEN rating number (0-255).\n");
        printf("  -ppr/--pegiptr: Optional. PEGI PTR rating number (0-255).\n");
        printf("  -pbr/--pegibbfc: Optional. PEGI BBFC rating number (0-255).\n");
        printf("  -cr/--cob: Optional. COB rating number (0-255).\n");
        printf("  -gr/--grb: Optional. GR rating number (0-255).\n");
        printf("  -cgr/--cgsrr: Optional. CGSRR rating number (0-255).\n");
    } else if(command.compare("makecwav") == 0) {
        printf("makecwav - Creates a CWAV file from a WAV.\n");
        printf("  -i/--input: WAV file to convert.\n");
        printf("  -o/--output: File to output the created CWAV to.\n");
    } else if(command.compare("lz11") == 0) {
        printf("lz11 - Compresses a file with LZ11.\n");
        printf("  -i/--input: File to compress.\n");
        printf("  -o/--output: File to output the compressed data to.\n");
    }
}

void cmd_print_commands() {
    printf("Available commands:\n");
    cmd_print_info("makebanner");
    cmd_print_info("makesmdh");
    cmd_print_info("makecwav");
    cmd_print_info("lz11");
}

void cmd_print_usage(const std::string& executedFrom) {
    printf("bannertool v%d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
    printf("Usage: %s <command> <args>\n", executedFrom.c_str());
    cmd_print_commands();
}

void cmd_missing_args(const std::string& command) {
    printf("Missing arguments for command \"%s\".\n", command.c_str());
    cmd_print_info(command);
}

void cmd_invalid_arg(const std::string& argument, const std::string& command) {
    printf("Invalid value for argument \"%s\" in command \"%s\".\n", argument.c_str(), command.c_str());
    cmd_print_info(command);
}

void cmd_invalid_command(const std::string& command) {
    printf("Invalid command \"%s\".\n", command.c_str());
    cmd_print_commands();
}

int cmd_process_command(int argc, char* argv[]) {
    if(argc < 2) {
        cmd_print_usage(argv[0]);
        return -1;
    }

    char* command = argv[1];
    std::map<std::string, std::string> args = cmd_get_args(argc, argv);
    if(strcmp(command, "makebanner") == 0) {
        const std::string banner = cmd_find_arg(args, "i", "image", "");
        const std::string audio = cmd_find_arg(args, "a", "audio", "");
        const std::string cgfxFile = cmd_find_arg(args, "ci", "cgfximage", "");
        const std::string cwavFile = cmd_find_arg(args, "ca", "cwavaudio", "");
        const std::string output = cmd_find_arg(args, "o", "output", "");
        if((banner.empty() && cgfxFile.empty()) || (audio.empty() && cwavFile.empty()) || output.empty()) {
            cmd_missing_args(command);
            return -1;
        }

        return cmd_make_banner(banner, audio, cgfxFile, cwavFile, output);
    } else if(strcmp(command, "makesmdh") == 0) {
        const std::string shortTitle = cmd_find_arg(args, "s", "shorttitle", "");
        const std::string longTitle = cmd_find_arg(args, "l", "longtitle", "");
        const std::string publisher = cmd_find_arg(args, "p", "publisher", "");
        const std::string icon = cmd_find_arg(args, "i", "icon", "");
        const std::string output = cmd_find_arg(args, "o", "output", "");
        if(shortTitle.empty() || longTitle.empty() || publisher.empty() || icon.empty() || output.empty()) {
            cmd_missing_args(command);
            return -1;
        }

        SMDH smdh;
        
        std::vector<std::string> regions = cmd_parse_list(cmd_find_arg(args, "r", "regions", "regionfree"));
        for(std::vector<std::string>::iterator it = regions.begin(); it != regions.end(); it++) {
            const std::string region = *it;
            if(region.compare("regionfree") == 0) {
                smdh.settings.regionLock = REGION_FREE;
                break;
            } else if(region.compare("japan") == 0) {
                smdh.settings.regionLock |= JAPAN;
            } else if(region.compare("northamerica") == 0) {
                smdh.settings.regionLock |= NORTH_AMERICA;
            } else if(region.compare("europe") == 0) {
                smdh.settings.regionLock |= EUROPE;
            } else if(region.compare("australia") == 0) {
                smdh.settings.regionLock |= AUSTRALIA;
            } else if(region.compare("china") == 0) {
                smdh.settings.regionLock |= CHINA;
            } else if(region.compare("korea") == 0) {
                smdh.settings.regionLock |= KOREA;
            } else if(region.compare("taiwan") == 0) {
                smdh.settings.regionLock |= TAIWAN;
            } else {
                cmd_invalid_arg("regions", command);
            }
        }

        std::vector<std::string> flags = cmd_parse_list(cmd_find_arg(args, "f", "flags", "visible,allow3d,recordusage"));
        for(std::vector<std::string>::iterator it = flags.begin(); it != flags.end(); it++) {
            const std::string flag = *it;
            if(flag.compare("visible") == 0) {
                smdh.settings.flags |= VISIBLE;
            } else if(flag.compare("autoboot") == 0) {
                smdh.settings.flags |= AUTO_BOOT;
            } else if(flag.compare("allow3d") == 0) {
                smdh.settings.flags |= ALLOW_3D;
            } else if(flag.compare("requireeula") == 0) {
                smdh.settings.flags |= REQUIRE_EULA;
            } else if(flag.compare("autosave") == 0) {
                smdh.settings.flags |= AUTO_SAVE_ON_EXIT;
            } else if(flag.compare("extendedbanner") == 0) {
                smdh.settings.flags |= USE_EXTENDED_BANNER;
            } else if(flag.compare("ratingrequired") == 0) {
                smdh.settings.flags |= RATING_REQUIED;
            } else if(flag.compare("savedata") == 0) {
                smdh.settings.flags |= USE_SAVE_DATA;
            } else if(flag.compare("recordusage") == 0) {
                smdh.settings.flags |= RECORD_USAGE;
            } else if(flag.compare("nosavebackups") == 0) {
                smdh.settings.flags |= DISABLE_SAVE_BACKUPS;
            } else if(flag.compare("new3ds") == 0) {
                smdh.settings.flags |= NEW_3DS;
            } else {
                cmd_invalid_arg("flags", command);
            }
        }

        u64 matchMakerId = (u64) atoll(cmd_find_arg(args, "mmid", "matchmakerid", "0").c_str());
        memcpy(smdh.settings.matchMakerId, &matchMakerId, 0xC);

        smdh.settings.eulaVersion = (u16) atoi(cmd_find_arg(args, "ev", "eulaversion", "0").c_str());
        smdh.settings.optimalBannerFrame = (u32) atoll(cmd_find_arg(args, "obf", "optimalbannerframe", "0").c_str());
        smdh.settings.streetpassId = (u32) atoll(cmd_find_arg(args, "spid", "streetpassid", "0").c_str());

        smdh.settings.gameRatings[CERO] = (u8) atoi(cmd_find_arg(args, "cer", "cero", "0").c_str());
        smdh.settings.gameRatings[ESRB] = (u8) atoi(cmd_find_arg(args, "er", "esrb", "0").c_str());
        smdh.settings.gameRatings[USK] = (u8) atoi(cmd_find_arg(args, "ur", "usk", "0").c_str());
        smdh.settings.gameRatings[PEGI_GEN] = (u8) atoi(cmd_find_arg(args, "pgr", "pegigen", "0").c_str());
        smdh.settings.gameRatings[PEGI_PTR] = (u8) atoi(cmd_find_arg(args, "ppr", "pegiptr", "0").c_str());
        smdh.settings.gameRatings[PEGI_BBFC] = (u8) atoi(cmd_find_arg(args, "pbr", "pegibbfc", "0").c_str());
        smdh.settings.gameRatings[COB] = (u8) atoi(cmd_find_arg(args, "cor", "cob", "0").c_str());
        smdh.settings.gameRatings[GRB] = (u8) atoi(cmd_find_arg(args, "gr", "grb", "0").c_str());
        smdh.settings.gameRatings[CGSRR] = (u8) atoi(cmd_find_arg(args, "cgr", "cgsrr", "0").c_str());

        return cmd_make_smdh(shortTitle, longTitle, publisher, icon, smdh, output);
    } else if(strcmp(command, "makecwav") == 0) {
        const std::string input = cmd_find_arg(args, "i", "input", "");
        const std::string output = cmd_find_arg(args, "o", "output", "");
        if(input.empty() || output.empty()) {
            cmd_missing_args(command);
            return -1;
        }

        return cmd_make_cwav(input, output);
    } else if(strcmp(command, "lz11") == 0) {
        const std::string input = cmd_find_arg(args, "i", "input", "");
        const std::string output = cmd_find_arg(args, "o", "output", "");
        if(input.empty() || output.empty()) {
            cmd_missing_args(command);
            return -1;
        }

        return cmd_lz11(input, output);
    } else {
        cmd_invalid_command(command);
        return -1;
    }
}
