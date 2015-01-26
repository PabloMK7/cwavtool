#ifndef __SMDH_H__
#define __SMDH_H__

#include "../types.h"

typedef enum {
    JAPANESE,
    ENGLISH,
    FRENCH,
    GERMAN,
    ITALIAN,
    SPANISH,
    SIMPLIFIED_CHINESE,
    KOREAN,
    DUTCH,
    PORTUGESE,
    RUSSIAN,
    TRADITIONAL_CHINESE
} SMDHTitleLanguage;

// TODO: Provide values to set ratings to.
typedef enum {
    CERO = 0,
    ESRB = 1,
    USK = 3,
    PEGI_GEN = 4,
    PEGI_PTR = 6,
    PEGI_BBFC = 7,
    COB = 8,
    GRB = 9,
    CGSRR = 10
} SMDHGameRating;

typedef enum {
    JAPAN = 0x01,
    NORTH_AMERICA = 0x02,
    EUROPE = 0x04,
    AUSTRALIA = 0x08,
    CHINA = 0x10,
    KOREA = 0x20,
    TAIWAN = 0x40,

    // Not a bitmask, but a value.
            REGION_FREE = 0x7FFFFFFF
} SMDHRegionFlag;

typedef enum {
    VISIBLE = 0x0001,
    AUTO_BOOT = 0x0002,
    ALLOW_3D = 0x0004,
    REQUIRE_EULA = 0x0008,
    AUTO_SAVE_ON_EXIT = 0x0010,
    USE_EXTENDED_BANNER = 0x0020,
    RATING_REQUIED = 0x0040,
    USE_SAVE_DATA = 0x0080,
    RECORD_USAGE = 0x0100,
    DISABLE_SAVE_BACKUPS = 0x0400
} SMDHFlag;

typedef struct {
    u16 shortDescription[0x40] = {0};
    u16 longDescription[0x80] = {0};
    u16 publisher[0x40] = {0};
} SMDHTitle;

typedef struct {
    u8 gameRatings[0x10] = {0};
    u32 regionLock = REGION_FREE;
    u8 matchMakerId[0xC] = {0};
    u32 flags = VISIBLE | ALLOW_3D | RECORD_USAGE;
    u16 eulaVersion = 0;
    u16 reserved1 = 0;
    u32 optimalBannerFrame = 0;
    u32 streetpassId = 0;
} SMDHSettings;

typedef struct {
    char magic[4] = {'S', 'M', 'D', 'H'};
    u16 version = 0;
    u16 reserved0 = 0;
    SMDHTitle titles[0x10];
    SMDHSettings settings;
    u64 reserved2 = 0;
    u8 smallIcon[0x480] = {0};
    u8 largeIcon[0x1200] = {0};
} SMDH;

#endif