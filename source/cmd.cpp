#include "cmd.h"

#include "3ds/3ds.h"
#include "pc/wav.h"
#include "types.h"

#include <string.h>

#include <map>
#include <vector>

u8* convert_to_cgfx(const char* image, u32 width, u32 height, u32* size) {
    u32 convertedSize = 0;
    u16* converted = image_to_tiles(image, width, height, RGBA4444, &convertedSize);
    if(converted == NULL) {
        return NULL;
    }

    u8* ret = (u8*) malloc(BANNER_CGFX_HEADER_LENGTH + convertedSize);
    memcpy(ret, BANNER_CGFX_HEADER, BANNER_CGFX_HEADER_LENGTH);
    memcpy(ret + BANNER_CGFX_HEADER_LENGTH, converted, convertedSize);

    *size = BANNER_CGFX_HEADER_LENGTH + convertedSize;
    return ret;
}

u8* convert_to_cwav(const char* file, u32* size) {
    WAV* wav = wav_read(file);
    if(!wav) {
        return NULL;
    }

    CWAV cwav;
    cwav.channels = wav->format.numChannels;
    cwav.sampleRate = wav->format.sampleRate;
    cwav.bitsPerSample = wav->format.bitsPerSample;
    cwav.dataSize = wav->data.chunkSize;
    cwav.data = wav->data.data;

    u8* ret = cwav_build(cwav, size);

    wav_free(wav);

    return ret;
}

int cmd_make_banner(const char* image, const char* audio, const char* cgfxFile, const char* cwavFile, const char* output) {
    u32 cgfxSize = 0;
    u8* cgfx = NULL;
    if(cgfxFile != NULL) {
        FILE* fd = fopen(cgfxFile, "r");
        if(!fd) {
            printf("ERROR: Could not open CGFX file: %s\n", strerror(errno));
        }

        fseek(fd, 0, SEEK_END);
        cgfxSize = (u32) ftell(fd);
        fseek(fd, 0, SEEK_SET);

        cgfx = (u8*) malloc(cgfxSize);
        fread(cgfx, 1, cgfxSize, fd);
        fclose(fd);
    } else {
        cgfx = convert_to_cgfx(image, 256, 128, &cgfxSize);
        if(!cgfx) {
            return 1;
        }
    }

    u32 cwavSize = 0;
    u8* cwav = NULL;
    if(cwavFile != NULL) {
        FILE* fd = fopen(cwavFile, "r");
        if(!fd) {
            printf("ERROR: Could not open CWAV file: %s\n", strerror(errno));
        }

        fseek(fd, 0, SEEK_END);
        cwavSize = (u32) ftell(fd);
        fseek(fd, 0, SEEK_SET);

        cwav = (u8*) malloc(cwavSize);
        fread(cwav, 1, cwavSize, fd);
        fclose(fd);
    } else {
        cwav = convert_to_cwav(audio, &cwavSize);
        if(!cwav) {
            return 2;
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

    FILE* fd = fopen(output, "wb");
    if(!fd) {
        printf("ERROR: Could not open output file: %s\n", strerror(errno));
        return 3;
    }

    fwrite(bnr, 1, bnrSize, fd);
    fclose(fd);

    free(bnr);

    printf("Created banner \"%s\".\n", output);
    return 0;
}

int cmd_make_smdh(const char* shortTitle, const char* longTitle, const char* publisher, const char* icon, SMDHRegionFlag regionFlags, u64 matchMakerId, u32 smdhFlags, u16 eulaVersion, u32 optimalBannerFrame, u32 streetpassId, const char* output) {
	u8* icon48Data = load_image(icon, 48, 48);
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

    SMDH smdh;
    for(int i = 0; i < 0x10; i++) {
        utf8_to_utf16(smdh.titles[i].shortTitle, shortTitle, 0x40);
        utf8_to_utf16(smdh.titles[i].longTitle, longTitle, 0x80);
        utf8_to_utf16(smdh.titles[i].publisher, publisher, 0x40);
    }

    smdh.settings.regionLock = regionFlags;
    memcpy(smdh.settings.matchMakerId, &matchMakerId, 0xC);
    smdh.settings.flags = smdhFlags;
    smdh.settings.eulaVersion = eulaVersion;
    smdh.settings.optimalBannerFrame = optimalBannerFrame;
    smdh.settings.streetpassId = streetpassId;

    memcpy(smdh.largeIcon, icon48, 0x1200);
    memcpy(smdh.smallIcon, icon24, 0x480);
    free(icon48);

    FILE* fd = fopen(output, "wb");
    if(!fd) {
        printf("ERROR: Could not open output file: %s\n", strerror(errno));
        return 2;
    }

    fwrite(&smdh, 1, sizeof(SMDH), fd);
    fclose(fd);

    printf("Created SMDH \"%s\".\n", output);
    return 0;
}

int cmd_make_cwav(const char* input, const char* output) {
    u32 cwavSize = 0;
    u8* cwav = convert_to_cwav(input, &cwavSize);
    if(!cwav) {
        return 1;
    }

    FILE* fd = fopen(output, "wb");
    if(!fd) {
        printf("ERROR: Could not open output file: %s\n", strerror(errno));
        return 2;
    }

    fwrite(cwav, 1, cwavSize, fd);
    fclose(fd);

    free(cwav);

    printf("Created CWAV \"%s\".\n", output);
    return 0;
}

int cmd_lz11(const char* input, const char* output) {
    FILE* in = fopen(input, "r");
    if(!in) {
        printf("ERROR: Could not open input file: %s\n", strerror(errno));
        return 1;
    }

    fseek(in, 0, SEEK_END);
    u32 size = (u32) ftell(in);
    fseek(in, 0, SEEK_SET);

    u8 data[size];
    fread(data, 1, size, in);
    fclose(in);

    u32 compressedSize;
    u8* compressed = lz11_compress(data, size, &compressedSize);
    if(!compressed) {
        return 2;
    }

    FILE* fd = fopen(output, "wb");
    if(!fd) {
        printf("ERROR: Could not open output file: %s\n", strerror(errno));
        return 3;
    }

    fwrite(compressed, 1, compressedSize, fd);
    fclose(fd);

    free(compressed);

    printf("Compressed to file \"%s\".\n", output);
    return 0;
}

struct compare_strings {
    bool operator()(char const *a, char const *b) {
        return strcmp(a, b) < 0;
    }
};

std::map<char*, char*, compare_strings> cmd_get_args(int argc, char* argv[]) {
    std::map<char*, char*, compare_strings> args;
    for(int i = 0; i < argc; i++) {
        if((strncmp(argv[i], "-", 1) == 0 || strncmp(argv[i], "--", 2) == 0) && argc != i + 1) {
            args.insert(std::pair<char*, char*>(argv[i], argv[i + 1]));
            i++;
        }
    }

    return args;
}

const char* cmd_find_arg(std::map<char*, char*, compare_strings> args, const char* shortOpt, const char* longOpt, const char* def) {
    char sopt[strlen(shortOpt) + 2];
    sprintf(sopt, "-%s", shortOpt);
    char lopt[strlen(longOpt) + 3];
    sprintf(lopt, "--%s", longOpt);

    std::map<char*, char*, compare_strings>::iterator match = args.find(sopt);
    if(match != args.end()) {
        return (*match).second;
    }

    match = args.find(lopt);
    if(match != args.end()) {
        return (*match).second;
    }

    return def;
}

std::vector<const char*> cmd_parse_list(const char* list) {
    std::vector<const char*> ret;
    const char* curr = list;
    const char* found = NULL;
    while((found = strchr(curr, ',')) != NULL) {
        char* substr = (char*) malloc(found - curr + 1);
        memcpy(substr, curr, found - curr);
        ret.push_back(substr);

        curr = found + 1;
    }

    if(strlen(curr) > 0) {
        ret.push_back(strdup(curr));
    }

    return ret;
}

void cmd_free_list(std::vector<const char*> list) {
    for(std::vector<const char*>::iterator it = list.begin(); it != list.end(); it++) {
        free((void*) *it);
    }
}

void cmd_print_info(const char* command) {
    if(strcmp(command, "makebanner") == 0) {
        printf("makebanner - Creates a .bnr file.\n");
        printf("  -i/--image: PNG file to use as the banner's image. Interchangeable with -ci.\n");
        printf("  -a/--audio: WAV file to use as the banner's tune. Interchangeable with -ca.\n");
        printf("  -ci/--cgfximage: CGFX file to use as the banner's image. Interchangeable with -i.\n");
        printf("  -ca/--cwavaudio: CWAV file to use as the banner's tune. Interchangeable with -a.\n");
        printf("  -o/--output: File to output the created banner to.\n");
    } else if(strcmp(command, "makesmdh") == 0) {
        printf("makesmdh - Creates a .smdh/.icn file.\n");
        printf("  -s/--shorttitle: Short title of the application.\n");
        printf("  -l/--longtitle: Long title of the application.\n");
        printf("  -p/--publisher: Publisher of the application.\n");
        printf("  -i/--icon: PNG file to use as an icon.\n");
        printf("  -r/--regions: Optional. Comma separated list of regions to lock the SMDH to.\n");
        printf("     Valid regions: regionfree, japan, northamerica, europe, australia, china, korea, taiwan.\n");
        printf("  -mmid/--matchmakerid: Optional. Match maker ID of the SMDH.\n");
        printf("  -f/--flags: Optional. Flags to apply to the SMDH file.\n");
        printf("     Valid flags: visible, autoboot, allow3d, requireeula, autosave, extendedbanner, ratingrequired, savedata, recordusage, nosavebackups.\n");
        printf("  -ev/--eulaversion: Optional. Version of the EULA required to be accepted before launching.\n");
        printf("  -obf/--optimalbannerframe: Optional. Optimal frame of the accompanying banner.\n");
        printf("  -spid/--streetpassid: Optional. Streetpass ID of the SMDH.\n");
        printf("  -o/--output: File to output the created SMDH/ICN to.\n");
    } else if(strcmp(command, "makecwav") == 0) {
        printf("makecwav - Creates a CWAV file from a WAV.\n");
        printf("  -i/--input: WAV file to convert.\n");
        printf("  -o/--output: File to output the created CWAV to.\n");
    } else if(strcmp(command, "lz11") == 0) {
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

void cmd_print_usage(const char* executedFrom) {
    printf("Usage: %s <command> <args>\n", executedFrom);
    cmd_print_commands();
}

void cmd_missing_args(const char* command) {
    printf("Missing arguments for command \"%s\".\n", command);
    cmd_print_info(command);
}

void cmd_invalid_arg(const char* argument, const char* command) {
    printf("Invalid value for argument \"%s\" in command \"%s\".\n", argument, command);
    cmd_print_info(command);
}

void cmd_invalid_command(const char* command) {
    printf("Invalid command \"%s\".\n", command);
    cmd_print_commands();
}

int cmd_process_command(int argc, char* argv[]) {
    if(argc < 2) {
        cmd_print_usage(argv[0]);
        return -1;
    }

    char* command = argv[1];
    std::map<char*, char*, compare_strings> args = cmd_get_args(argc, argv);
    if(strcmp(command, "makebanner") == 0) {
        const char *banner = cmd_find_arg(args, "i", "image", NULL);
        const char *audio = cmd_find_arg(args, "a", "audio", NULL);
        const char *cgfxFile = cmd_find_arg(args, "ci", "cgfximage", NULL);
        const char *cwavFile = cmd_find_arg(args, "ca", "cwavaudio", NULL);
        const char *output = cmd_find_arg(args, "o", "output", NULL);
        if(!(banner || cgfxFile) || !(audio || cwavFile) || !output) {
            cmd_missing_args(command);
            return -1;
        }

        return cmd_make_banner(banner, audio, cgfxFile, cwavFile, output);
    } else if(strcmp(command, "makesmdh") == 0) {
        const char* shortTitle = cmd_find_arg(args, "s", "shorttitle", NULL);
        const char* longTitle = cmd_find_arg(args, "l", "longtitle", NULL);
        const char* publisher = cmd_find_arg(args, "p", "publisher", NULL);
        const char* icon = cmd_find_arg(args, "i", "icon", NULL);
        const char* output = cmd_find_arg(args, "o", "output", NULL);
        if(!shortTitle || !longTitle || !publisher || !icon || !output) {
            cmd_missing_args(command);
            return -1;
        }

        std::vector<const char*> regions = cmd_parse_list(cmd_find_arg(args, "r", "regions", "regionfree"));
        u64 matchMakerId = (u64) atoll(cmd_find_arg(args, "mmid", "matchmakerid", "0"));
        std::vector<const char*> flags = cmd_parse_list(cmd_find_arg(args, "f", "flags", "visible,allow3d,recordusage"));
        u16 eulaVersion = (u16) atoi(cmd_find_arg(args, "ev", "eulaversion", "0"));
        u32 optimalBannerFrame = (u32) atoll(cmd_find_arg(args, "obf", "optimalbannerframe", "0"));
        u32 streetpassId = (u32) atoll(cmd_find_arg(args, "spid", "streetpassid", "0"));

        u32 regionFlags = 0;
        for(std::vector<const char*>::iterator it = regions.begin(); it != regions.end(); it++) {
            const char* region = *it;
            if(strcmp(region, "regionfree") == 0) {
                regionFlags = REGION_FREE;
                break;
            } else if(strcmp(region, "japan") == 0) {
                regionFlags |= JAPAN;
            } else if(strcmp(region, "northamerica") == 0) {
                regionFlags |= NORTH_AMERICA;
            } else if(strcmp(region, "europe") == 0) {
                regionFlags |= EUROPE;
            } else if(strcmp(region, "australia") == 0) {
                regionFlags |= AUSTRALIA;
            } else if(strcmp(region, "china") == 0) {
                regionFlags |= CHINA;
            } else if(strcmp(region, "korea") == 0) {
                regionFlags |= KOREA;
            } else if(strcmp(region, "taiwan") == 0) {
                regionFlags |= TAIWAN;
            } else {
                cmd_invalid_arg("regions", command);
            }
        }

        u32 smdhFlags = 0;
        for(std::vector<const char*>::iterator it = flags.begin(); it != flags.end(); it++) {
            const char* flag = *it;
            if(strcmp(flag, "visible") == 0) {
                smdhFlags |= VISIBLE;
            } else if(strcmp(flag, "autoboot") == 0) {
                smdhFlags |= AUTO_BOOT;
            } else if(strcmp(flag, "allow3d") == 0) {
                smdhFlags |= ALLOW_3D;
            } else if(strcmp(flag, "requireeula") == 0) {
                smdhFlags |= REQUIRE_EULA;
            } else if(strcmp(flag, "autosave") == 0) {
                smdhFlags |= AUTO_SAVE_ON_EXIT;
            } else if(strcmp(flag, "extendedbanner") == 0) {
                smdhFlags |= USE_EXTENDED_BANNER;
            } else if(strcmp(flag, "ratingrequired") == 0) {
                smdhFlags |= RATING_REQUIED;
            } else if(strcmp(flag, "savedata") == 0) {
                smdhFlags |= USE_SAVE_DATA;
            } else if(strcmp(flag, "recordusage") == 0) {
                smdhFlags |= RECORD_USAGE;
            } else if(strcmp(flag, "nosavebackups") == 0) {
                smdhFlags |= DISABLE_SAVE_BACKUPS;
            } else {
                cmd_invalid_arg("flags", command);
            }
        }

        cmd_free_list(regions);
        cmd_free_list(flags);

        return cmd_make_smdh(shortTitle, longTitle, publisher, icon, (SMDHRegionFlag) regionFlags, matchMakerId, smdhFlags, eulaVersion, optimalBannerFrame, streetpassId, output);
    } else if(strcmp(command, "makecwav") == 0) {
        const char* input = cmd_find_arg(args, "i", "input", NULL);
        const char* output = cmd_find_arg(args, "o", "output", NULL);
        if(!input || !output) {
            cmd_missing_args(command);
            return -1;
        }

        return cmd_make_cwav(input, output);
    } else if(strcmp(command, "lz11") == 0) {
        const char* input = cmd_find_arg(args, "i", "input", NULL);
        const char* output = cmd_find_arg(args, "o", "output", NULL);
        if(!input || !output) {
            cmd_missing_args(command);
            return -1;
        }

        return cmd_lz11(input, output);
    } else {
        cmd_invalid_command(command);
        return -1;
    }
}