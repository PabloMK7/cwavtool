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

typedef struct {
    u16 shortDescription[0x40] = {0};
    u16 longDescription[0x80] = {0};
    u16 publisher[0x40] = {0};
} SMDHTitle;

typedef struct {
    // TODO: values...
    u8 cero = 0;
    u8 esrb = 0;
    u8 reserved0 = 0;
    u8 usk = 0;
    u8 pegiGen = 0;
    u8 reserved1 = 0;
    u8 pegiPrt = 0;
    u8 pegiBbfc = 0;
    u8 cob = 0;
    u8 grb = 0;
    u8 cgsrr = 0;
    u8 reserved2 = 0;
    u8 reserved3 = 0;
    u8 reserved4 = 0;
    u8 reserved5 = 0;
    u8 reserved6 = 0;
} SMDHGameRatings;

typedef struct _region_lock {
    _region_lock() : japan(true), northAmerica(true), europe(true), australia(true), china(true), korea(true), taiwan(true) {}

    bool japan : 1;
    bool northAmerica : 1;
    bool europe : 1;
    bool australia : 1;
    bool china : 1;
    bool korea : 1;
    bool taiwan : 1;
} SMDHRegionLock;

typedef struct _flags {
    _flags() : visible(true), autoBoot(false), allow3d(true), requireEula(false), autoSaveOnExit(false), useExtendedBanner(false), ratingRequired(false), useSaveData(false), recordUsage(true), disableSaveBackups(false) {}

    bool visible : 1;
    bool autoBoot : 1;
    bool allow3d : 1;
    bool requireEula : 1;
    bool autoSaveOnExit : 1;
    bool useExtendedBanner : 1;
    bool ratingRequired : 1;
    bool useSaveData : 1;
    bool recordUsage : 1;
    bool disableSaveBackups : 1;
} SMDHFlags;

typedef struct {
    SMDHGameRatings gameRatings;
    SMDHRegionLock regionLock;
    u8 matchMakerId[0xC] = {0};
    SMDHFlags flags;
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