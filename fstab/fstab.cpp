/*
 * Copyright (C) 2008 The Android Open Source Project
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
#include <cutils/android_reboot.h>
#include <cutils/properties.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#define LOG_TAG "fstab"
#include <cutils/log.h>

#include "fstab.h"

/* Load STORAGE_CONFIG_PROP from PERSISTENT_PROPERTY_DIR
 * We need to get it earlier than vendor init to make working fstab */
static void load_storage_config_prop() {
    std::unique_ptr<DIR, int(*)(DIR*)> dir(opendir(PERSISTENT_PROPERTY_DIR), closedir);
    if (!dir) {
        ERROR("Unable to open persistent property directory \"%s\": %s\n",
              PERSISTENT_PROPERTY_DIR, strerror(errno));
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir.get())) != NULL) {
        if (strncmp(STORAGE_CONFIG_PROP, entry->d_name, strlen(STORAGE_CONFIG_PROP))) {
            continue;
        }
        if (entry->d_type != DT_REG) {
            continue;
        }

        // Open the file and read the property value.
        int fd = openat(dirfd(dir.get()), entry->d_name, O_RDONLY | O_NOFOLLOW);
        if (fd == -1) {
            ERROR("Unable to open persistent property file \"%s\": %s\n",
                  entry->d_name, strerror(errno));
            continue;
        }

        struct stat sb;
        if (fstat(fd, &sb) == -1) {
            ERROR("fstat on property file \"%s\" failed: %s\n", entry->d_name, strerror(errno));
            close(fd);
            continue;
        }

        // File must not be accessible to others, be owned by root/root, and
        // not be a hard link to any other file.
        if (((sb.st_mode & (S_IRWXG | S_IRWXO)) != 0) || (sb.st_uid != 0) || (sb.st_gid != 0) ||
                (sb.st_nlink != 1)) {
            ERROR("skipping insecure property file %s (uid=%u gid=%u nlink=%u mode=%o)\n",
                  entry->d_name, (unsigned int)sb.st_uid, (unsigned int)sb.st_gid,
                  (unsigned int)sb.st_nlink, sb.st_mode);
            close(fd);
            continue;
        }

        char value[PROP_VALUE_MAX];
        int length = read(fd, value, sizeof(value) - 1);
        if (length >= 0) {
            value[length] = 0;
            ERROR("%s: property_set(%s, %s)\n", __func__, entry->d_name, value);
            property_set(entry->d_name, value);
        } else {
            ERROR("Unable to read persistent property file %s: %s\n",
                  entry->d_name, strerror(errno));
        }
        close(fd);
    }
}

/* Check if partition exist on given sdcc
 * Return TRUE if exist, FALSE if not */
static int check_for_partition(int sdcc, const char *part_name)
{
    char full_part_name[PROP_VALUE_MAX] = { '\0' };
    if (sdcc == RAWDEV)
        snprintf(full_part_name, PROP_VALUE_MAX, "/dev/block/%s", part_name);
    else
        snprintf(full_part_name, PROP_VALUE_MAX, "/dev/block/platform/msm_sdcc.%d/by-name/%s", sdcc, part_name);
    ERROR("%s: INFO: Checking for partition '%s'\n", __func__, full_part_name);

    if (access(full_part_name, F_OK) == 0)
        return TRUE;

    return FALSE;
}

/* Lookup for partition part_name on storage devices in search_order
 * Return newly allocated buffer with full path or NULL if partition not found */
static char *lookup_for_partition(const char *part_name, int search_order) {
    int sdcc = -1;
    int partition_is_found = FALSE;
    char *full_part_name = NULL;

    switch (search_order) {

        case RAWDEV:
            if (check_for_partition(RAWDEV, part_name))
                partition_is_found = true;
            break;

        case SDCC_1:
            if (check_for_partition(SDCC_1, part_name)) {
                partition_is_found = true;
                sdcc = SDCC_1;
            }
            break;

        case SDCC_2:
            if (check_for_partition(SDCC_2, part_name)) {
                partition_is_found = true;
                sdcc = SDCC_2;
            }
            break;

        case SDCC_1_2:
            if (check_for_partition(SDCC_1, part_name)) {
                partition_is_found = true;
                sdcc = SDCC_1;
            } else if (check_for_partition(SDCC_2, part_name)) {
                partition_is_found = true;
                sdcc = SDCC_2;
            }
            break;

        case SDCC_2_1:
            if (check_for_partition(SDCC_2, part_name)) {
                partition_is_found = true;
                sdcc = SDCC_2;
            } else if (check_for_partition(SDCC_1, part_name)) {
                partition_is_found = true;
                sdcc = SDCC_1;
            }
            break;

        case JUST_ADD_IT:
            partition_is_found = true;
            break;

        default:
            ERROR("%s: ERROR: invalid search order %d passed!", __func__, search_order);
            return NULL;
            break;
    }

    if (!partition_is_found) {
        ERROR("%s: WARNING: partition for '%s' NOT FOUND!", __func__, part_name);
        return NULL;
    }

    full_part_name = (char *)calloc(PROP_VALUE_MAX, sizeof(char));
    if (!full_part_name) {
        ERROR("%s: ERROR: calloc() failed\n", __func__);
        return NULL;
    }

    switch (search_order) {
        case RAWDEV:
            snprintf(full_part_name, PROP_VALUE_MAX, "/dev/block/%s", part_name);
            break;
        case JUST_ADD_IT:
            snprintf(full_part_name, PROP_VALUE_MAX, "%s", part_name);
            break;
        default:
            snprintf(full_part_name, PROP_VALUE_MAX, "/dev/block/platform/msm_sdcc.%d/by-name/%s", sdcc, part_name);
            break;
    }

    ERROR("%s: INFO: using partition '%s'", __func__, full_part_name);
    return full_part_name;
}

/* Get minor number of partition device node
 * Return -1 on failure */
static int get_partition_number(const char *part_name)
{
    int part_number = -1;
    char raw_blockdev_name[PROP_VALUE_MAX] = {0};
    int sdcc = -1;
    if (sscanf(part_name, "/devices/msm_sdcc.%d/mmc_host*", &sdcc) != 1)
        return -1;


    char *full_part_name = lookup_for_partition("usbmsc", sdcc);
    if (full_part_name)
        readlink(full_part_name, raw_blockdev_name, PROP_VALUE_MAX);

    ERROR("%s: sdcc=%d (%s): %s\n", __func__, sdcc, full_part_name, raw_blockdev_name);

    if (strlen(raw_blockdev_name)) {
        if (sscanf(raw_blockdev_name, "/dev/block/mmcblk%dp%d", &sdcc, &part_number) != 2) {
            ERROR("%s: sscanf() failed: %d(%s)\n", __func__, errno, strerror(errno));
            return -1;
        }
        ERROR("%s: part_number for %s on sdcc%d is %d\n", __func__, full_part_name, sdcc, part_number);
    }

    free(full_part_name);
    return part_number;
}

/* Add partition to fstab, if exist
 * Return 0 on success, negative value if failed */
static int add_fstab_entry(
    int fd,
    int t,
    int search_order,
    const char *part_name,
    const char *mnt_point,
    const char *type,
    const char *mnt_flags,
    const char *fs_mgr_flags)
{
    int ret = FALSE;
    char *full_part_name = lookup_for_partition(part_name, search_order);

    if (!full_part_name) {
        ERROR("%s: ERROR: partition '%s' NOT FOUND!", __func__, part_name);
        return -1;
    }

    int partition_number = get_partition_number(part_name);
    char fs_mgr_flags_fixed[PROP_VALUE_MAX] = {0};
    snprintf(fs_mgr_flags_fixed, PROP_VALUE_MAX, fs_mgr_flags, partition_number);

    switch (t) {
        case FSTAB_TYPE_REGULAR:
        case FSTAB_TYPE_RECOVERY:
            ret = dprintf(fd, "%s %s %s %s %s\n", full_part_name, mnt_point, type, mnt_flags, fs_mgr_flags_fixed);
            break;
        case FSTAB_TYPE_TWRP:
            ret = dprintf(fd, "%s %s %s %s %s\n", mnt_point, type, full_part_name, mnt_flags, fs_mgr_flags_fixed);
            break;
        default:
            ERROR("%s: ERROR: invalid partition type %d!", __func__, t);
            ret = -1;
            break;
    }
    free(full_part_name);
    return (ret < 0 ? ret : 0);
}

static int generate_regular_fstab(int fd, int type, int sdcc_config)
{
    int ret = 0;
    (void)sdcc_config;
    dprintf(fd, FSTAB_HEADER);
    ret += add_fstab_entry(fd, type, SDCC_1, "system",   "/system",   "ext4", "ro,barrier=1", "wait");
    ret += add_fstab_entry(fd, type, SDCC_1, "cache",    "/cache",    "f2fs", "rw,nosuid,nodev,noatime,nodiratime,inline_xattr", "wait,check");
    ret += add_fstab_entry(fd, type, SDCC_1, "cache",    "/cache",    "ext4", "noatime,nosuid,nodev,barrier=1", "wait,check,formattable");
    ret += add_fstab_entry(fd, type, SDCC_1, "userdata", "/data",     "f2fs", "rw,nosuid,nodev,noatime,nodiratime,inline_xattr", "wait,check,encryptable=footer");
    ret += add_fstab_entry(fd, type, SDCC_1, "userdata", "/data",     "ext4", "noatime,nosuid,nodev,barrier=1,noauto_da_alloc", "wait,check,formattable,encryptable=footer");
    ret += add_fstab_entry(fd, type, SDCC_1, "persist",  "/persist",  "ext4", "nosuid,nodev,barrier=1", "wait");
    ret += add_fstab_entry(fd, type, SDCC_1, "modem",    "/firmware", "vfat", "ro,shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337,context=u:object_r:firmware_file:s0", "wait");
    ret += add_fstab_entry(fd, type, SDCC_1, "boot",     "/boot",     "emmc", "defaults", "defaults");
    ret += add_fstab_entry(fd, type, SDCC_1, "recovery", "/recovery", "emmc", "defaults", "defaults");
    ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/platform/msm_hsusb_host/usb", "/storage/usbdisk", "vfat", "nosuid,nodev", "voldmanaged=usbdisk:auto");
    return (ret == 0 ? TRUE : FALSE);
}

static int generate_recovery_fstab(int fd, int type, int sdcc_config)
{
    int ret = 0;
    (void)sdcc_config;
    dprintf(fd, FSTAB_HEADER);
    ret += add_fstab_entry(fd, type, SDCC_1, "system",   "/system",   "ext4", "rw,barrier=1", "wait"); // rw here is requied to avoid issues with TWRP!
    ret += add_fstab_entry(fd, type, SDCC_1, "cache",    "/cache",    "f2fs", "rw,nosuid,nodev,noatime,nodiratime,inline_xattr", "wait,check");
    ret += add_fstab_entry(fd, type, SDCC_1, "cache",    "/cache",    "ext4", "noatime,nosuid,nodev,barrier=1", "wait,check,formattable");
    ret += add_fstab_entry(fd, type, SDCC_1, "userdata", "/data",     "f2fs", "rw,nosuid,nodev,noatime,nodiratime,inline_xattr", "wait,check,encryptable=footer");
    ret += add_fstab_entry(fd, type, SDCC_1, "userdata", "/data",     "ext4", "noatime,nosuid,nodev,barrier=1,noauto_da_alloc", "wait,check,formattable,encryptable=footer");
    ret += add_fstab_entry(fd, type, SDCC_1, "persist",  "/persist",  "ext4", "nosuid,nodev,barrier=1", "wait");
    ret += add_fstab_entry(fd, type, SDCC_1, "modem",    "/firmware", "vfat", "rw,shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337,context=u:object_r:firmware_file:s0", "wait"); // rw here is requied to avoid issues with TWRP!
    ret += add_fstab_entry(fd, type, SDCC_1, "boot",     "/boot",     "emmc", "defaults", "defaults");
    ret += add_fstab_entry(fd, type, SDCC_1, "recovery", "/recovery", "emmc", "defaults", "defaults");
    ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/platform/msm_hsusb_host/usb", "/storage/usbdisk", "vfat", "nosuid,nodev", "voldmanaged=usbdisk:auto");
    return (ret == 0 ? TRUE : FALSE);
}

static int generate_twrp_fstab(int fd, int type, int sdcc_config)
{
    int ret = 0;
    dprintf(fd, FSTAB_HEADER);
    ret += add_fstab_entry(fd, type, SDCC_1, "cache",    "/cache",    "ext4", "", "");
    ret += add_fstab_entry(fd, type, SDCC_1, "system",   "/system",   "ext4", "", "");
    ret += add_fstab_entry(fd, type, SDCC_1, "userdata", "/data",     "f2fs", "length=-16384;canencryptbackup=1;userdataencryptbackup=1", "");
    ret += add_fstab_entry(fd, type, SDCC_1, "boot",     "/boot",     "emmc", "", "");
    ret += add_fstab_entry(fd, type, SDCC_1, "recovery", "/recovery", "emmc", "flags=display=\"Recovery\";backup=1", "");
    ret += add_fstab_entry(fd, type, SDCC_1, "modem",    "/modem",    "emmc", "flags=display=\"Modem\";backup=1", "");
    ret += add_fstab_entry(fd, type, SDCC_1, "persist",  "/persist",  "emmc", "flags=display=\"Persist\";backup=1", "");
    ret += add_fstab_entry(fd, type, SDCC_1, "splash",   "/splash",   "emmc", "flags=display=\"Splash\";backup=1", "");
    ret += add_fstab_entry(fd, type, SDCC_1, "modemst1", "/efs1",     "emmc", "flags=backup=1;display=\"EFS\"", "");
    ret += add_fstab_entry(fd, type, SDCC_1, "modemst2", "/efs2",     "emmc", "flags=backup=1;subpartitionof=/efs1", "");
    ret += add_fstab_entry(fd, type, SDCC_1, "fsg",      "/fsg",      "emmc", "flags=backup=1;subpartitionof=/efs1", "");
    ret += add_fstab_entry(fd, type, SDCC_1, "fsc",      "/fsc",      "emmc", "flags=backup=1;subpartitionof=/efs1", "");
    ret += add_fstab_entry(fd, type, SDCC_1, "sdi",      "/sdi",      "emmc", "flags=backup=1;display=\"SDI\"", "");
    ret += add_fstab_entry(fd, type, SDCC_1, "misc",     "/misc",     "emmc", "", "");
    ERROR("%s: sdcc_config = %d\n", __func__, sdcc_config);
    switch (sdcc_config) {
        case REGULAR:
            ERROR("%s: sdcc_config = regular, adding mmcblk0\n", __func__);
            ret += add_fstab_entry(fd, type, RAWDEV, "mmcblk0",     "/full", "emmc", "flags=backup=1;display=\"eMMC\"", "");
            ret += add_fstab_entry(fd, type, RAWDEV, "mmcblk0rpmb", "/rpmb", "emmc", "flags=backup=1;subpartitionof=/full", "");
            break;
        case INVERTED:
            ERROR("%s: sdcc_config = inverted, adding mmcblk1\n", __func__);
            ret += add_fstab_entry(fd, type, RAWDEV, "mmcblk1",     "/full", "emmc", "flags=backup=1;display=\"eMMC\"", "");
            ret += add_fstab_entry(fd, type, RAWDEV, "mmcblk1rpmb", "/rpmb", "emmc", "flags=backup=1;subpartitionof=/full", "");
            break;
        case ISOLATED:
            ERROR("%s: sdcc_config = isolated, not ading\n", __func__);
            break;

        default:
            ERROR("%s: sdcc_config = default!!\n", __func__);
            break;
    }
    return (ret < 0 ? 1 : 0);
}

static int update_regular_fstab(int fd, int type, int storage_config, int sdcc_config)
{
    int ret = 0;
    (void)sdcc_config;
    ERROR("%s: stprage_config=%d, sdcc_config=%d\n", __func__, storage_config, sdcc_config);
    switch (storage_config) {
        case STORAGE_CONFIGURATION_CLASSIC:
            switch (sdcc_config) {
                case REGULAR:
                    ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:%d,noemulatedsd,nonremovable");
                    if (check_for_partition(SDCC_2, "usbmsc"))
                        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d,encryptable=userdata");
                    else
                        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:auto,encryptable=userdata");
                   break;
                case INVERTED:
                    ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:%d,noemulatedsd,nonremovable");
                    ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d,nonremovable,encryptable=userdata");
                    break;
                case ISOLATED:
                    ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:%d,noemulatedsd,nonremovable");
                    break;
                default:
                    break;
            }
            break;
        case STORAGE_CONFIGURATION_INVERTED:
            switch (sdcc_config) {
                case REGULAR:
                    if (check_for_partition(SDCC_2, "usbmsc"))
                        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:%d,noemulatedsd");
                    else
                        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:auto,noemulatedsd");
                    ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d,nonremovable,encryptable=userdata");
                   break;
                case INVERTED:
                    ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:%d,noemulatedsd,nonremovable");
                    ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d,nonremovable,encryptable=userdata");
                    break;
                case ISOLATED:
                    ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:%d,noemulatedsd,nonremovable");
                    break;
                default:
                    break;
            }
            break;
        case STORAGE_CONFIGURATION_DATAMEDIA:
            switch (sdcc_config) {
                case REGULAR:
                    ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d,nonremovable,noemulatedsd,encryptable=userdata");
                    if (check_for_partition(SDCC_2, "usbmsc"))
                        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard2:%d,encryptable=userdata");
                    else
                        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard2:auto,encryptable=userdata");
                   break;
                case INVERTED:
                    ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d,nonremovable,noemulatedsd,encryptable=userdata");
                    ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard2:%d,nonremovable,encryptable=userdata");
                    break;
                case ISOLATED:
                    ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d,noemulatedsd,nonremovable,encryptable=userdata");
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return ret;
}

static int update_recovery_fstab(int fd, int type, int storage_config, int sdcc_config)
{
    return (update_regular_fstab(fd, type, storage_config, sdcc_config)); // STUB for now
}

static int update_twrp_fstab(int fd, int type, int storage_config, int sdcc_config)
{
    int ret = 0;
    (void)sdcc_config;
    switch (storage_config) {
        case STORAGE_CONFIGURATION_CLASSIC:
        case STORAGE_CONFIGURATION_INVERTED:
            switch (sdcc_config) {
                case REGULAR:
                    ret += add_fstab_entry(fd, type, SDCC_1,     "usbmsc",    "/internal_sd", "vfat", "flags=display=\"Internal SD\";storagename=\"Internal SD\";storage;settingsstorage;wipeingui;fsflags=utf8", "");
                    if (check_for_partition(SDCC_2, "usbmsc"))
                        ret += add_fstab_entry(fd, type, SDCC_2, "usbmsc",    "/external_sd", "vfat", "flags=display=\"SD card\";storagename=\"SD card\";storage;wipeingui;removable;fsflags=utf8", "");
                    else
                        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/dev/block/mmcblk1p1 /dev/block/mmcblk1", "/external_sd", "vfat", "flags=display=\"SD card\";storagename=\"SD card\";storage;wipeingui;removable;fsflags=utf8", "");
                    break;
                case INVERTED:
                    ret += add_fstab_entry(fd, type, SDCC_2,     "usbmsc"   , "/internal_sd", "vfat", "flags=display=\"Internal SD\";storagename=\"Internal SD\";storage;settingsstorage;wipeingui;fsflags=utf8", "");
                    if (check_for_partition(SDCC_1, "usbmsc"))
                        ret += add_fstab_entry(fd, type, SDCC_1, "usbmsc",    "/external_sd", "vfat", "flags=display=\"SD card\";storagename=\"SD card\";storage;wipeingui;removable;fsflags=utf8", "");
                    else
                        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/dev/block/mmcblk1p1 /dev/block/mmcblk1", "/external_sd", "vfat", "flags=display=\"SD card\";storagename=\"SD card\";storage;wipeingui;removable;fsflags=utf8", "");
                    break;
                    ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/dev/block/mmcblk1p1 /dev/block/mmcblk1", "/external_sd", "vfat", "flags=display=\"SD card\";storagename=\"SD card\";storage;wipeingui;removable;fsflags=utf8", "");
                    break;
                case ISOLATED:
                    ret += add_fstab_entry(fd, type, SDCC_1, "usbmsc", "/external_sd", "vfat", "flags=display=\"SD card\";storagename=\"SD card\";storage;wipeingui;removable;fsflags=utf8,settingsstorage", "");
                    break;
            }
            break;
        case STORAGE_CONFIGURATION_DATAMEDIA:
            switch (sdcc_config) {
                case REGULAR:
                    if (check_for_partition(SDCC_2, "usbmsc"))
                        ret += add_fstab_entry(fd, type, SDCC_1, "usbmsc", "/external_sd", "vfat", "flags=display=\"SD card\";storagename=\"SD card\";storage;wipeingui;removable;fsflags=utf8", "");
                    else
                        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/dev/block/mmcblk1p1 /dev/block/mmcblk1", "/external_sd", "vfat", "flags=display=\"SD card\";storagename=\"SD card\";storage;wipeingui;removable;fsflags=utf8", "");
                    break;
                case INVERTED:
                    ret += add_fstab_entry(fd, type, SDCC_2, "usbmsc", "/internal_sd", "vfat", "flags=display=\"Internal SD\";storagename=\"Internal SD\";storage;settingsstorage;wipeingui;fsflags=utf8", "");
                    ret += add_fstab_entry(fd, type, SDCC_1, "usbmsc", "/external_sd", "vfat", "flags=display=\"SD card\";storagename=\"SD card\";storage;wipeingui;removable;fsflags=utf8", "");
                    break;
                case ISOLATED:
                    ret += add_fstab_entry(fd, type, SDCC_1, "usbmsc", "/external_sd", "vfat", "flags=display=\"SD card\";storagename=\"SD card\";storage;wipeingui;removable;fsflags=utf8", "");
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    ret += add_fstab_entry(fd, type, JUST_ADD_IT,  "/dev/block/sda1 /dev/block/sda", "/usb-otg", "vfat", "flags=display=\"USB OTG\";storagename=\"USB OTG\";storage;wipeingui;removable;fsflags=utf8", "");
    return ret;
}

static void remount_rootfs(unsigned long *flags)
{
    unsigned long mount_flags=0;
    int error = 0;
    struct statvfs statvfs_buf;
    errno = 0;
    if (statvfs("/init", &statvfs_buf) != 0)
        ERROR("statvfs() failed, errno: %d (%s)\n", errno, strerror(errno));
    else
        mount_flags=statvfs_buf.f_flag;
    ERROR("Remounting / with flags=%lu\n", *flags);
    errno = 0;
    error = mount("rootfs", "/", "rootfs", MS_REMOUNT|*flags, NULL);
    if (error)
        ERROR("WARNING: Remounting / with flags=%lu FAILED (%s), mount returned %d\n", *flags, strerror(errno), error);
    *flags=mount_flags;
    return;
}

int process_fstab(const char *fstab_name, int fstab_type, int fstab_action)
{
    int counter = 0;
    char config[PROP_VALUE_MAX] = {0};
    unsigned long flags = 0;
    int fd = -1, ret = 0;
    int storage_config = STORAGE_CONFIGURATION_CLASSIC;
    int sdcc_config    = REGULAR;
    ERROR("%s: entered with name=%s type=%d action=%d\n", __func__, fstab_name, fstab_type, fstab_action);

    remount_rootfs(&flags);

    errno=0;
    while (access(COLDBOOT_DONE, F_OK) != 0) {
        NOTICE("Waiting for coldboot marker '%s', counter=%d, errno=%s!\n", COLDBOOT_DONE, counter, strerror(errno));
        usleep(100);
        counter++;
    }

    load_storage_config_prop();
    property_get("ro.boot.swap_sdcc", config, "");
    sdcc_config = atoi(config);
    property_get("persist.storages.configuration", config, "");
    storage_config = atoi(config);
    ERROR("storage_config=%d, sdcc_config=%d, '%s'\n", storage_config, sdcc_config, config);

    errno = 0;
    if (fstab_action == FSTAB_ACTION_GENERATE) {
        ERROR("Generating fstab '%s', type %d\n", fstab_name, fstab_type);
        fd = open(fstab_name, O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC, 0600);
    }
    else
    {
        ERROR("Updating fstab '%s', type %d\n", fstab_name, fstab_type);
        fd = open(fstab_name, O_WRONLY|O_CLOEXEC, 0600);
        lseek(fd, 0l, SEEK_END);
    }

    if (fd >= 0) {
        ERROR("opened '%s' (fd=%d)\n", fstab_name, fd);
        switch (fstab_type) {
            case FSTAB_TYPE_REGULAR:
                if (fstab_action == FSTAB_ACTION_GENERATE)
                    ret = generate_regular_fstab(fd, fstab_type, sdcc_config);
                if (fstab_action == FSTAB_ACTION_UPDATE)
                    ret = update_regular_fstab(fd, fstab_type, storage_config, sdcc_config);
                break;
            case FSTAB_TYPE_RECOVERY:
                if (fstab_action == FSTAB_ACTION_GENERATE)
                    ret = generate_recovery_fstab(fd, fstab_type, sdcc_config);
                if (fstab_action == FSTAB_ACTION_UPDATE)
                    ret = update_recovery_fstab(fd, fstab_type, storage_config, sdcc_config);
                break;
            case FSTAB_TYPE_TWRP:
                if (fstab_action == FSTAB_ACTION_GENERATE)
                    ret = generate_twrp_fstab(fd, fstab_type, sdcc_config);
                if (fstab_action == FSTAB_ACTION_UPDATE)
                    ret = update_twrp_fstab(fd, fstab_type, storage_config, sdcc_config);
                break;
            default:
                ERROR("Error: Unknown fstab type (%d)\n", fstab_type);

        }
         close(fd);
    } else {
        ERROR("could not open '%s' (%s)\n", fstab_name, strerror(errno));
        goto failure;
    }
    remount_rootfs(&flags);
    return(ret);

    failure:
        remount_rootfs(&flags);
        android_reboot(ANDROID_RB_RESTART2, 0, "recovery");
        while (1) { pause(); }  // never reached
}

int main(int nargs, char **args)
{
    const char *fstab_name = NULL;
    int fstab_type     = FSTAB_TYPE_REGULAR;
    int fstab_action   = FSTAB_ACTION_GENERATE;
    int ret = FALSE;
    ERROR("%s: entered with name=%s type=%s action=%s\n", __func__, args[1], args[2], args[3]);
    if (nargs == 4 && args[1] && args[2] && args[3]) {
        fstab_name = args[1];
        if (!strncmp(args[2],"regular", sizeof("regular")))
            fstab_type = FSTAB_TYPE_REGULAR;
        if (!strncmp(args[2],"recovery", sizeof("recovery")))
            fstab_type = FSTAB_TYPE_RECOVERY;
        if (!strncmp(args[2],"twrp", sizeof("twrp")))
            fstab_type = FSTAB_TYPE_TWRP;
        if (!strncmp(args[3],"generate", sizeof("generate")))
            fstab_action = FSTAB_ACTION_GENERATE;
        if (!strncmp(args[3],"update", sizeof("update")))
            fstab_action = FSTAB_ACTION_UPDATE;
        ret = process_fstab(fstab_name, fstab_type, fstab_action);
    } else {
        ERROR("%s: invalid arguments (nargs=%d, args[0]=%s, args[1]=%s, args[2]=%s, args[3]=%s\n", __func__, nargs, args[0], args[1], args[2], args[3]);
    }
    return ret;
}
