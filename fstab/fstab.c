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
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#define LOG_TAG "fstab"
#include <cutils/log.h>

#include "fstab.h"
#include "configuration.h"

// Do not write anything to the fstab file
static int dry_run = FALSE;

/* Load STORAGE_CONFIG_PROP from PERSISTENT_PROPERTY_DIR
 * We need to get it earlier than vendor init to make working fstab */
static void load_storage_config_prop() {
    DIR* dir = opendir(PERSISTENT_PROPERTY_DIR);
    if (!dir) {
        ERROR("Unable to open persistent property directory \"%s\": %s\n",
              PERSISTENT_PROPERTY_DIR, strerror(errno));
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(STORAGE_CONFIG_PROP, entry->d_name, strlen(STORAGE_CONFIG_PROP)) != 0) {
            continue;
        }
        if (entry->d_type != DT_REG) {
            continue;
        }

        // Open the file and read the property value.
        int fd = openat(dirfd(dir), entry->d_name, O_RDONLY | O_NOFOLLOW);
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

    if (access(full_part_name, F_OK) == 0) {
        ERROR("%s: INFO: Checking for partition '%s': TRUE\n", __func__, full_part_name);
        return TRUE;
    }
    ERROR("%s: INFO: Checking for partition '%s': FALSE\n", __func__, full_part_name);
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
                partition_is_found = TRUE;
            break;

        case SDCC_1:
            if (check_for_partition(SDCC_1, part_name)) {
                partition_is_found = TRUE;
                sdcc = SDCC_1;
            }
            break;

        case SDCC_2:
            if (check_for_partition(SDCC_2, part_name)) {
                partition_is_found = TRUE;
                sdcc = SDCC_2;
            }
            break;

        case SDCC_1_2:
            if (check_for_partition(SDCC_1, part_name)) {
                partition_is_found = TRUE;
                sdcc = SDCC_1;
            } else if (check_for_partition(SDCC_2, part_name)) {
                partition_is_found = TRUE;
                sdcc = SDCC_2;
            }
            break;

        case SDCC_2_1:
            if (check_for_partition(SDCC_2, part_name)) {
                partition_is_found = TRUE;
                sdcc = SDCC_2;
            } else if (check_for_partition(SDCC_1, part_name)) {
                partition_is_found = TRUE;
                sdcc = SDCC_1;
            }
            break;

        case JUST_ADD_IT:
            partition_is_found = TRUE;
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
    char partition[PROP_VALUE_MAX] = {0};
    int sdcc = -1;
    if (sscanf(part_name, "/devices/msm_sdcc.%d/mmc_host*", &sdcc) == 1) {
        ERROR("%s: recursively detecting partition usbmsc on sdcc%d\n", __func__, sdcc);
        snprintf(raw_blockdev_name, PROP_VALUE_MAX, "/dev/block/platform/msm_sdcc.%d/by-name/usbmsc", sdcc);
        return get_partition_number(raw_blockdev_name);
    }

    if (sscanf(part_name, "/dev/block/platform/msm_sdcc.%d/by-name/%s", &sdcc, partition) == 2) {
      char *full_part_name = lookup_for_partition(partition, sdcc);
      if (full_part_name){
        if (readlink(full_part_name, raw_blockdev_name, PROP_VALUE_MAX) == -1) {
          ERROR("%s: readlink() failed for %s (%s)\n", __func__, full_part_name, strerror(errno));
          return -2;
        }
      } else {
        ERROR("%s: lookup_for_partition() returned NULL for %s on sdcc %d\n", __func__, partition, sdcc);
        return -3;
      }

      ERROR("%s: sdcc=%d (%s): %s\n", __func__, sdcc, full_part_name, raw_blockdev_name);

      if (strlen(raw_blockdev_name)) {
        if (sscanf(raw_blockdev_name, "/dev/block/mmcblk%dp%d", &sdcc, &part_number) != 2) {
          ERROR("%s: sscanf() failed: %d(%s)\n", __func__, errno, strerror(errno));
          return -1;
        }
        sdcc += 1;
        ERROR("%s: part_number for %s on sdcc%d is %d\n", __func__, full_part_name, sdcc, part_number);
      }

      free(full_part_name);
      return part_number;
    }

    if (!strncmp(part_name, "/devices/platform/msm_hsusb_host/usb", strlen("/devices/platform/msm_hsusb_host/usb"))) {
        ERROR("%s: %s is the USB host device, so returning 0\n", __func__, part_name);
        return 0;
    }

    ERROR("%s: Should not reach here!\n", __func__);
    return -4;
}

/* Add partition to fstab, if exist
 * Return 0 on success, negative value if failed */
static int add_fstab_entry(
    int fd,
    int fstab_type,
    int search_order,
    const char *part_name,
    const char *mnt_point,
    const char *type,
    const char *mnt_flags,
    const char *fs_mgr_flags)
{
    if (dry_run)
        return TRUE;

    int ret = FALSE;
    char *full_part_name = lookup_for_partition(part_name, search_order);

    if (!full_part_name) {
        ERROR("%s: ERROR: partition '%s' NOT FOUND!", __func__, part_name);
        return -1;
    }

    int partition_number = get_partition_number(full_part_name);
    char fs_mgr_flags_fixed[PROP_VALUE_MAX] = {0};
    snprintf(fs_mgr_flags_fixed, PROP_VALUE_MAX, fs_mgr_flags, partition_number);

    switch (fstab_type) {
        case FSTAB_TYPE_REGULAR:
        case FSTAB_TYPE_RECOVERY:
            ret = dprintf(fd, "%s %s %s %s %s\n", full_part_name, mnt_point, type, mnt_flags, fs_mgr_flags_fixed);
            break;
        case FSTAB_TYPE_TWRP:
            ret = dprintf(fd, "%s %s %s %s %s\n", mnt_point, type, full_part_name, mnt_flags, fs_mgr_flags_fixed);
            break;
        default:
            ERROR("%s: ERROR: invalid fstab type %d!", __func__, fstab_type);
            ret = -1;
            break;
    }
    free(full_part_name);
    return (ret < 0 ? ret : 0);
}

static void emmc_has_usbmsc(void) {
  ERROR(__func__);
  property_set(EMMC_HAS_USBMSC_PROP, "true");
}

static void emmc_has_no_usbmsc(void) {
  ERROR(__func__);
  property_set(EMMC_HAS_USBMSC_PROP, "false");
}

static void sd_has_usbmsc(void) {
  ERROR(__func__);
  property_set(SD_HAS_USBMSC_PROP, "true");
}

static void sd_has_no_usbmsc(void) {
  ERROR(__func__);
  property_set(SD_HAS_USBMSC_PROP, "false");
}

static void sd_has_plain_part(void) {
  ERROR(__func__);
  property_set(SD_HAS_PLAIN_PART_PROP, "true");
}

static void sd_has_no_plain_part(void) {
  ERROR(__func__);
  property_set(SD_HAS_PLAIN_PART_PROP, "false");
}

// Flags:
// encryptable=userdata - Makes storage adoptable.
//                        Should not be used on bootable SD (because Android will repartition and reformat it)
// nonremovable - do not show notifications on volume status changes
// noemulatedsd - marks primary storage

static int update_regular_classic(int fd, int type, int sdcc_config) {
  int ret = 0;
  ERROR(__func__);
  switch (sdcc_config) {
    case REGULAR:
      if (check_for_partition(SDCC_1, "usbmsc")) {
        emmc_has_usbmsc();
        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:%d,noemulatedsd,nonremovable");
        if (check_for_partition(SDCC_2, "usbmsc")) {
          sd_has_usbmsc();
          ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d");
        } else if (check_for_partition(SDCC_2, "shared")) {
          // SD was partitioned with sm partition disk:179,64 mixed command
          sd_has_usbmsc();
          ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d");
        } else {
          if ( !check_for_partition(SDCC_2, "system") && (check_for_partition(SDCC_2, "shared") || !check_for_partition(SDCC_2, "android_expand")) ) {
            // SD is not bootable, have "shared" partition (have "mixed" semi-adopted state) or not adopted, so it should be considered as having plain partition
            sd_has_plain_part();
            ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:auto,encryptable=userdata");
          }
        }
      } else {
        if (check_for_partition(SDCC_2, "usbmsc")) {
          sd_has_usbmsc();
          ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:%d,noemulatedsd");
        } else {
          if ( !check_for_partition(SDCC_2, "system") && (check_for_partition(SDCC_2, "shared") || !check_for_partition(SDCC_2, "android_expand")) ) {
            // SD is not bootable, have "shared" partition (have "mixed" semi-adopted state) or not adopted, so it should be considered as having plain partition
            sd_has_plain_part();
            ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:auto,encryptable=userdata");
          }
        }
      }
      break;
    case INVERTED:
      if (check_for_partition(SDCC_1, "usbmsc")) {
        sd_has_usbmsc();
        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:%d,noemulatedsd,nonremovable");
        if (check_for_partition(SDCC_2, "usbmsc")) {
          emmc_has_usbmsc();
          ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d,nonremovable");
        }
      } else {
        if (check_for_partition(SDCC_2, "usbmsc")) { // usbmsc on eMMC may be only numbered partition, not "auto"
          emmc_has_usbmsc();
          ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:%d,noemulatedsd,nonremovable");
        }
      }
      break;
    case ISOLATED:
      if (check_for_partition(SDCC_1, "usbmsc")) {
        sd_has_usbmsc();
        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:%d,noemulatedsd,nonremovable");
      }
      break;
    default:
      break;
  }
  return ret;
}

static int update_regular_inverted(int fd, int type, int sdcc_config) {
  int ret = 0;
  ERROR(__func__);
  switch (sdcc_config) {
    case REGULAR:
      if (check_for_partition(SDCC_2, "usbmsc")) {
        sd_has_usbmsc();
        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:%d,noemulatedsd");
      } else {
        if ( !check_for_partition(SDCC_2, "system") && (check_for_partition(SDCC_2, "shared") || !check_for_partition(SDCC_2, "android_expand")) ) {
          // SD is not bootable, have "shared" partition (have "mixed" semi-adopted state) or not adopted, so it should be considered as having plain partition
          sd_has_plain_part();
          ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:auto,noemulatedsd,encryptable=userdata");
        }
      }
      if (check_for_partition(SDCC_1, "usbmsc")) {
        emmc_has_usbmsc();
        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d,nonremovable");
      }
      break;
    case INVERTED:
      if (check_for_partition(SDCC_1, "usbmsc")) {
        sd_has_usbmsc();
        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:%d,noemulatedsd,nonremovable");
        if (check_for_partition(SDCC_2, "usbmsc")) {
          emmc_has_usbmsc();
          ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d,nonremovable");
        }
      } else {
        if (check_for_partition(SDCC_2, "usbmsc")) {
          emmc_has_usbmsc();
          ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:%d,noemulatedsd,nonremovable");
        }
      }
      break;
    case ISOLATED:
      if (check_for_partition(SDCC_1, "usbmsc")) {
        sd_has_usbmsc();
        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard0:%d,noemulatedsd,nonremovable");
      } else
      break;
    default:
      break;
  }
  return ret;
}

static int update_regular_datamedia(int fd, int type, int sdcc_config) {
  int ret = 0;
  ERROR(__func__);
    switch (sdcc_config) {
    case REGULAR:
      if (check_for_partition(SDCC_1, "usbmsc")) {
        emmc_has_usbmsc();
        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d,nonremovable");
        if (check_for_partition(SDCC_2, "usbmsc")) {
          sd_has_usbmsc();
          ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard2:%d");
        } else {
          if ( !check_for_partition(SDCC_2, "system") && (check_for_partition(SDCC_2, "shared") || !check_for_partition(SDCC_2, "android_expand")) ) {
            // SD is not bootable, have "shared" partition (have "mixed" semi-adopted state) or not adopted, so it should be considered as having plain partition
            sd_has_plain_part();
            ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard2:auto,encryptable=userdata");
          }
        }
      } else {
        if (check_for_partition(SDCC_2, "usbmsc")) {
          sd_has_usbmsc();
          ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d");
        } else {
          if ( !check_for_partition(SDCC_2, "system") && (check_for_partition(SDCC_2, "shared") || !check_for_partition(SDCC_2, "android_expand")) ) {
            // SD is not bootable, have "shared" partition (have "mixed" semi-adopted state) or not adopted, so it should be considered as having plain partition
            sd_has_plain_part();
            ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:auto,encryptable=userdata");
          }
        }
      }
      break;
    case INVERTED:
      if (check_for_partition(SDCC_1, "usbmsc")) {
        sd_has_usbmsc();
        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d,nonremovable");
        if (check_for_partition(SDCC_2, "usbmsc")) {
          emmc_has_usbmsc();
          ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard2:%d,nonremovable");
        }
      } else {
        if (check_for_partition(SDCC_2, "usbmsc")) {
          emmc_has_usbmsc();
          ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.2/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d,nonremovable");
        }
      }
      break;
    case ISOLATED:
      if (check_for_partition(SDCC_1, "usbmsc")) {
        sd_has_usbmsc();
        ret += add_fstab_entry(fd, type, JUST_ADD_IT, "/devices/msm_sdcc.1/mmc_host*", "auto", "auto", "defaults", "voldmanaged=sdcard1:%d,nonremovable");
      }
      break;
    default:
      break;
  }
  return ret;
}

static int update_regular_fstab(int fd, int type, int storage_config, int sdcc_config)
{
    int ret = 0;
    ERROR(__func__);
    ERROR("%s: storage_config=%d, sdcc_config=%d\n", __func__, storage_config, sdcc_config);

    if (sdcc_config == REGULAR) {
      if (!check_for_partition(RAWDEV, "mmcblk1")) {
        ERROR("%s: SD card not found", __func__);
        sd_has_no_usbmsc();
        sd_has_no_plain_part();
      }
    }

    // SD have no plain partition if it is bootable
    if (sdcc_config == INVERTED || sdcc_config == ISOLATED)
      sd_has_no_plain_part();

    // We have no access to eMMC usbmsc partition in ISOLATED sdcc configuration
    if (sdcc_config == ISOLATED)
      emmc_has_no_usbmsc();

    // Perform dry-run pass to set up properties
    dry_run = TRUE;

    switch (storage_config) {
        case STORAGE_CONFIGURATION_CLASSIC:
            update_regular_classic(fd, type, sdcc_config);
            break;
        case STORAGE_CONFIGURATION_INVERTED:
            update_regular_inverted(fd, type, sdcc_config);
            break;
        case STORAGE_CONFIGURATION_DATAMEDIA:
            update_regular_datamedia(fd, type, sdcc_config);
            break;
        default:
            break;
    }

    // this will fail if props was set already - ro.* props can be set only once
    emmc_has_no_usbmsc();
    sd_has_no_usbmsc();
    sd_has_no_plain_part();

    char tmp[PROP_VALUE_MAX] = {0};
    property_get(EMMC_HAS_USBMSC_PROP, tmp, "false");
    int emmc_usbmsc = strcmp(tmp, "false");
    property_get(SD_HAS_USBMSC_PROP, tmp, "false");
    int sd_usbmsc = strcmp(tmp, "false");
    property_get(SD_HAS_PLAIN_PART_PROP, tmp, "false");
    int sd_plain_part= strcmp(tmp, "false");

    if (storage_config == STORAGE_CONFIGURATION_CLASSIC && !emmc_usbmsc) {
      ERROR("%s: storage_config=%d is invalid - no usbmsc partition on eMMC found, fixing\n", __func__, storage_config);
      if (sd_usbmsc || sd_plain_part) {
        ERROR("%s: SD card can become primary storage, using it\n", __func__);
        storage_config = STORAGE_CONFIGURATION_INVERTED;
        sprintf(tmp, "%d", STORAGE_CONFIGURATION_INVERTED);
        property_set(STORAGE_CONFIG_PROP, tmp);
      } else {
        ERROR("%s: SD card can't become primary storage, using datamedia\n", __func__);
        storage_config = STORAGE_CONFIGURATION_DATAMEDIA;
        sprintf(tmp, "%d", STORAGE_CONFIGURATION_DATAMEDIA);
        property_set(STORAGE_CONFIG_PROP, tmp);
      }
      ERROR("%s: now storage_config=%d\n", __func__, storage_config);
    }

    if (storage_config == STORAGE_CONFIGURATION_INVERTED && !sd_usbmsc && !sd_plain_part) {
      ERROR("%s: storage_config=%d is invalid - no usbmsc or plain data partition on SD found, fixing\n", __func__, storage_config);
      if (emmc_usbmsc) {
        ERROR("%s: eMMC usbmsc can become primary storage, using it\n", __func__);
        storage_config = STORAGE_CONFIGURATION_CLASSIC;
        sprintf(tmp, "%d", STORAGE_CONFIGURATION_CLASSIC);
        property_set(STORAGE_CONFIG_PROP, tmp);
      } else {
        ERROR("%s: eMMC usbmsc can't become primary storage, using datamedia\n", __func__);
        storage_config = STORAGE_CONFIGURATION_DATAMEDIA;
        sprintf(tmp, "%d", STORAGE_CONFIGURATION_DATAMEDIA);
        property_set(STORAGE_CONFIG_PROP, tmp);
      }
      ERROR("%s: now storage_config=%d\n", __func__, storage_config);
    }

    // Perform real update pass
    dry_run = FALSE;
    switch (storage_config) {
      case STORAGE_CONFIGURATION_CLASSIC:
        ret = update_regular_classic(fd, type, sdcc_config);
        break;
      case STORAGE_CONFIGURATION_INVERTED:
        ret = update_regular_inverted(fd, type, sdcc_config);
        break;
      case STORAGE_CONFIGURATION_DATAMEDIA:
        ret = update_regular_datamedia(fd, type, sdcc_config);
        break;
      default:
        break;
    }
    return ret;
}

static int update_recovery_fstab(int fd, int type, int storage_config, int sdcc_config)
{
    ERROR(__func__);
    return (update_regular_fstab(fd, type, storage_config, sdcc_config)); // STUB for now
}

static int update_twrp_regular(int fd, int type, int sdcc_config) {
  int ret = 0;
  ERROR(__func__);
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
    case ISOLATED:
      ret += add_fstab_entry(fd, type, SDCC_1, "usbmsc", "/external_sd", "vfat", "flags=display=\"SD card\";storagename=\"SD card\";storage;wipeingui;removable;fsflags=utf8,settingsstorage", "");
      break;
  }
  return ret;
}

static int update_twrp_datamedia(int fd, int type, int sdcc_config) {
  int ret = 0;
  ERROR(__func__);
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
  return ret;
}

static int update_twrp_fstab(int fd, int type, int storage_config, int sdcc_config)
{
    int ret = 0;
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
        ERROR("%s: sdcc_config = isolated, not ading eMMC backup partition record\n", __func__);
        break;
      default:
        ERROR("%s: sdcc_config = default!!\n", __func__);
        break;
    }

    switch (storage_config) {
        case STORAGE_CONFIGURATION_CLASSIC:
        case STORAGE_CONFIGURATION_INVERTED:
            update_twrp_regular(fd, type, sdcc_config);
            break;
        case STORAGE_CONFIGURATION_DATAMEDIA:
            update_twrp_datamedia(fd, type, sdcc_config);
            break;
        default:
            break;
    }
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

static void print_fstab(const char *fstab_name)
{
  FILE *mf = fopen(fstab_name, "r");
  if (!mf) {
    ERROR("%d: mf not opened: %s", __LINE__,strerror(errno));
    return;
  }
  while (1)
   {
      char str[512];
      char *estr = fgets (str,sizeof(str),mf);
      if (estr == NULL)
      {
         if ( feof (mf) != 0)
         {
            ERROR("INFO: End of fstab\n");
            break;
         }
         else
         {
            ERROR("ERROR: Error reading fstab: %s\n", strerror(errno));
            break;
         }
      }
      ERROR("%s\n", str);
   }
   fclose(mf);
}

int process_fstab(const char *fstab_name, const int fstab_type)
{
    int counter = 0;
    char config[PROP_VALUE_MAX] = {0};
    unsigned long flags = 0;
    int fd = -1, ret = 0;
    int storage_config = STORAGE_CONFIGURATION_CLASSIC;
    int sdcc_config    = REGULAR;
    ERROR("%s: entered with name=%s type=%d\n", __func__, fstab_name, fstab_type);

    remount_rootfs(&flags);

    errno=0;
    while (access(COLDBOOT_DONE, F_OK) != 0) {
        NOTICE("Waiting for coldboot marker '%s', counter=%d, errno=%s!\n", COLDBOOT_DONE, counter, strerror(errno));
        usleep(100);
        counter++;
    }

    ERROR("%s: Loading storages configuration", __func__);
    load_storage_config_prop();
    property_get("ro.boot.swap_sdcc", config, "");
    sdcc_config = atoi(config);
    property_get("persist.storages.configuration", config, "");
    storage_config = atoi(config);
    ERROR("storage_config=%d, sdcc_config=%d, '%s'\n", storage_config, sdcc_config, config);

    ERROR("Updating fstab '%s', type %d\n", fstab_name, fstab_type);
    fd = open(fstab_name, O_WRONLY|O_CLOEXEC, 0600);
    lseek(fd, 0l, SEEK_END);

    if (fd < 0) {
        ERROR("could not open '%s' (%s)\n", fstab_name, strerror(errno));
        goto failure;
    }

    ERROR("opened '%s' (fd=%d)\n", fstab_name, fd);
    ERROR("FSTAB before processing:\n");
    print_fstab(fstab_name);

    switch (fstab_type) {
        case FSTAB_TYPE_REGULAR:
            ret = update_regular_fstab(fd, fstab_type, storage_config, sdcc_config);
            break;
        case FSTAB_TYPE_RECOVERY:
            ret = update_recovery_fstab(fd, fstab_type, storage_config, sdcc_config);
            break;
        case FSTAB_TYPE_TWRP:
            ret = update_twrp_fstab(fd, fstab_type, storage_config, sdcc_config);
            break;
        default:
            ERROR("Error: Unknown fstab type (%d)\n", fstab_type);
    }

    ERROR("FSTAB after processing:\n");
    print_fstab(fstab_name);
    close(fd);

    remount_rootfs(&flags);
    return(ret);

    failure:
        remount_rootfs(&flags);
        android_reboot(ANDROID_RB_RESTART2, 0, "recovery");
        while (1) { pause(); }  // never reached
}

int main(int nargs, char **args)
{
    int fstab_type = FSTAB_TYPE_REGULAR;
    if (!(nargs == 3 && args[1] && args[2])) {
        ERROR("%s: invalid arguments (nargs=%d, args[0]=%s, args[1]=%s, args[2]=%s\n", __func__, nargs, args[0], args[1], args[2]);
        return FALSE;
    }
    ERROR("%s: entered with name=%s type=%s\n", __func__, args[1], args[2]);
    const char *fstab_name = args[1];
    if (!strncmp(args[2],"regular", sizeof("regular")))
        fstab_type = FSTAB_TYPE_REGULAR;
    if (!strncmp(args[2],"recovery", sizeof("recovery")))
        fstab_type = FSTAB_TYPE_RECOVERY;
    if (!strncmp(args[2],"twrp", sizeof("twrp")))
        fstab_type = FSTAB_TYPE_TWRP;
    int ret = process_fstab(fstab_name, fstab_type);
    set_storage_props();
    return ret;
}
