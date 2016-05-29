# TARGET #

TARGET := NATIVE
LIBRARY := 0

ifeq ($(TARGET),3DS)
    ifeq ($(strip $(DEVKITPRO)),)
        $(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
    endif

    ifeq ($(strip $(DEVKITARM)),)
        $(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
    endif
endif

# COMMON CONFIGURATION #

NAME := bannertool

BUILD_DIR := build
OUTPUT_DIR := output
INCLUDE_DIRS := include
SOURCE_DIRS := source

EXTRA_OUTPUT_FILES :=

LIBRARY_DIRS :=
LIBRARIES :=

BUILD_FLAGS := -Wno-unused-result -Wno-sign-compare
RUN_FLAGS :=

# 3DS CONFIGURATION #

TITLE := $(NAME)
DESCRIPTION :=
AUTHOR :=
PRODUCT_CODE :=
UNIQUE_ID :=

SYSTEM_MODE :=
SYSTEM_MODE_EXT :=

ICON_FLAGS :=

ROMFS_DIR :=
BANNER_AUDIO :=
BANNER_IMAGE :=
ICON :=

# INTERNAL #

include buildtools/make_base
