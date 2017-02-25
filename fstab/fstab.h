/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _INIT_FSTAB_H
#define _INIT_FSTAB_H
#include "log.h"

#define FALSE 0
#define TRUE !FALSE
#define FSTAB_HEADER \
"# Android fstab file.\n\
# The filesystem that contains the filesystem checker binary (typically /system) cannot\n\
# specify MF_CHECK, and must come before any filesystems that do specify MF_CHECK\n\
\n\
#<src> <mnt_point> <type> <mnt_flags and options> <fs_mgr_flags>\n"

// From system/core/init/util.h
#define COLDBOOT_DONE "/dev/.coldboot_done"

enum fstab_types
{
    FSTAB_TYPE_REGULAR = 0,
    FSTAB_TYPE_RECOVERY,
    FSTAB_TYPE_TWRP,
    FSTAB_TYPES
};

enum fstab_actions
{
    FSTAB_ACTION_GENERATE = 0,
    FSTAB_ACTION_UPDATE,
    FSTAB_ACTIONS
};

enum storage_configurations
{
    STORAGE_CONFIGURATION_CLASSIC = 0,
    STORAGE_CONFIGURATION_INVERTED,
    STORAGE_CONFIGURATION_DATAMEDIA,
    STORAGE_CONFIGURATIONS
};

enum partition_search_order
{
        RAWDEV = 0,
        SDCC_1,
        SDCC_2,
        SDCC_1_2,
        SDCC_2_1,
        JUST_ADD_IT
};

enum sdcc_configs
{
        REGULAR = 0,
        INVERTED,
        ISOLATED
};

#define STORAGE_CONFIG_PROP "persist.storages.configuration"
#define PERSISTENT_PROPERTY_DIR  "/data/property"

#endif //_INIT_FSTAB_H
