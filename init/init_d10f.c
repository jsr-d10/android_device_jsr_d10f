#include <stdlib.h>
#include <stdio.h>

#include "vendor_init.h"
#include "property_service.h"
#include "log.h"
#include "util.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mount.h>


#include "init_msm.h"

#define PERSISTENT_PROPERTY_DIR  "/data/property"
#define PERSISTENT_PROPERTY_PLANNED_SWAP    "persist.storages.planned_swap"
#define PERSISTENT_PROPERTY_SWAPPED         "persist.storages.swapped"
#define RO_PROPERTY_SWAP_SDCC               "ro.boot.swap_sdcc"

static const char * prop_forced_list[] = {
	PERSISTENT_PROPERTY_PLANNED_SWAP,
};


#define FILE_FSTAB           "/fstab.d10f"
#define FILE_FSTAB_TEMPLATE  "/fstab.template"
#define FILE_STORAGE_LIST    "/data/system/storage_list.xml"

#define STR_STORAGE_INTERNAL "@string/storage_internal"
#define STR_STORAGE_SDCARD   "@string/storage_sd_card"
#define STR_STORAGE_USB      "@string/storage_usb"

#define STOR_PRIMARY         0
#define STOR_SECONDARY       1
#define STOR_USBDISK         2

#define PATR_NUM_AUTO        (-1)
#define PATR_NUM_USBMSC      16

struct mount_point {
	const char * name;
	const char * path;
};

static struct mount_point mnt_point[] = {
	{
		.name = "sdcard0",
		.path = "/storage/sdcard0",
	},
	{
		.name = "sdcard1",
		.path = "/storage/sdcard1",
	},
	{
		.name = "usbdisk",
		.path = "/storage/usbdisk",
	},
};

struct storage_item {
	int emmc;
	int sdcc;
	int part_num;
	const char * storageDescription;
	int removable;
	int emulated;
	int allowMtp;
	int allowMassStorage;
	int maxFileSize;
};

static struct storage_item storage_list[] = {
	{
		.emmc = 1,
		.sdcc = 1,
		.part_num = PATR_NUM_AUTO,
		.storageDescription = STR_STORAGE_INTERNAL,
		.removable = 0,
		.allowMassStorage = 1,
		.allowMtp = 1,
	},
	{
		.emmc = 0,
		.sdcc = 2,
		.part_num = PATR_NUM_AUTO,
		.storageDescription = STR_STORAGE_SDCARD,
		.removable = 1,
		.allowMassStorage = 1,
		.allowMtp = 1,
	},
	{
		.emmc = 0,
		.sdcc = 3,
		.part_num = PATR_NUM_AUTO,
		.storageDescription = STR_STORAGE_USB,
		.removable = 1,
		.allowMassStorage = 0,
		.allowMtp = 0,
	},
};

int stor_swapped = 0;
int swap_sdcc = 0;


int check_prop_forced(const char * prop_name)
{
	size_t i;
	size_t len = strlen(prop_name);
	for (i = 0; i < ARRAY_SIZE(prop_forced_list); i++) {
		if (prop_forced_list[i] && strlen(prop_forced_list[i]) == len) {
			if (strcmp(prop_forced_list[i], prop_name) == 0) {
				return 1; 
			}
		}
	}
	return 0;
}

int init_prop_forced_list(void)
{
	char value[PROP_VALUE_MAX];
	DIR * dir;
	int dir_fd;
	struct dirent * entry;
	int fd, length;
	struct stat sb;

	dir = opendir(PERSISTENT_PROPERTY_DIR);
	if (dir) {
		dir_fd = dirfd(dir);
		while ((entry = readdir(dir)) != NULL) {
			// we need to read this properties before load_persistent_properties()
			if (!check_prop_forced(entry->d_name))
				continue;
#if HAVE_DIRENT_D_TYPE
			if (entry->d_type != DT_REG)
				continue;
#endif
			/* open the file and read the property value */
			fd = openat(dir_fd, entry->d_name, O_RDONLY | O_NOFOLLOW);
			if (fd < 0) {
				ERROR("Unable to open persistent property file \"%s\" errno: %d\n", entry->d_name, errno);
				continue;
			}
			if (fstat(fd, &sb) < 0) {
				ERROR("fstat on property file \"%s\" failed errno: %d\n", entry->d_name, errno);
				close(fd);
				continue;
			}

			// File must not be accessible to others, be owned by root/root, and
			// not be a hard link to any other file.
			if (((sb.st_mode & (S_IRWXG | S_IRWXO)) != 0)
				      || (sb.st_uid != 0)
				      || (sb.st_gid != 0)
				      || (sb.st_nlink != 1)) {
				ERROR("skipping insecure property file %s (uid=%u gid=%u nlink=%d mode=%o)\n",
				      entry->d_name, (unsigned int)sb.st_uid, (unsigned int)sb.st_gid,
				      sb.st_nlink, sb.st_mode);
				close(fd);
				continue;
			}

			length = read(fd, value, sizeof(value) - 1);
			if (length >= 0) {
				value[length] = 0;
				property_set(entry->d_name, value);
			} else {
				ERROR("Unable to read persistent property file %s errno: %d\n", entry->d_name, errno);
			}
			close(fd);
		}
		closedir(dir);
	} else {
		ERROR("Unable to open persistent property directory %s errno: %d\n", PERSISTENT_PROPERTY_DIR, errno);
	}
	return 0;
}

/*
// frameworks/base/core/res/res/values/attrs.xml
    <declare-styleable name="Storage">
        <!-- path to mount point for the storage -->
        <attr name="mountPoint" format="string" />
        <!-- user visible description of the storage -->
        <attr name="storageDescription" format="string" />
        <!-- true if the storage is the primary external storage -->
        <attr name="primary" format="boolean" />
        <!-- true if the storage is removable -->
        <attr name="removable" format="boolean" />
        <!-- true if the storage is emulated via the FUSE sdcard daemon -->
        <attr name="emulated" format="boolean" />
        <!-- number of megabytes of storage MTP should reserve for free storage
             (used for emulated storage that is shared with system's data partition) -->
        <attr name="mtpReserve" format="integer" />
        <!-- make false if don't want to get mounted on MTP -->
        <attr name="allowMtp" format="boolean" />
        <!-- true if the storage can be shared via USB mass storage -->
        <attr name="allowMassStorage" format="boolean" />
        <!-- maximum file size for the volume in megabytes, zero or unspecified if it is unbounded -->
        <attr name="maxFileSize" format="integer" />
    </declare-styleable>

// frameworks/base/services/core/java/com/android/server/MountService.java
        public String path = null;
        public int descriptionId = -1;
        public String description = null;
        public boolean primary = false;
        public boolean removable = false;
        public boolean emulated = false;
        public int mtpReserve = 0;
        public boolean allowMassStorage = false;
        public boolean allowMtp = false;
        public long maxFileSize = 0;

*/

int create_storage_list(void)
{
	size_t i;
	FILE * f = fopen(FILE_STORAGE_LIST, "w+");
	if (f == NULL) {
		ERROR("can not open '%s', err: %s\n", FILE_STORAGE_LIST, strerror(errno));
		return -1;
	}
	fprintf(f, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
	fprintf(f, "<StorageList>\n");
	for (i = 0; i < ARRAY_SIZE(storage_list); i++) {
		struct storage_item * s = &storage_list[i];
		fprintf(f, "  <storage mountPoint=\"%s\" \n", mnt_point[i].path);
		fprintf(f, "    storageDescription=\"%s\" \n", s->storageDescription);
		if (i == STOR_PRIMARY) 		fprintf(f, "    primary=\"true\" \n");
		if (s->removable) 		fprintf(f, "    removable=\"true\" \n");
		if (s->emulated) 		fprintf(f, "    emulated=\"true\" \n");
		if (s->allowMassStorage) 	fprintf(f, "    allowMassStorage=\"true\" \n");
		if (s->allowMtp) 		fprintf(f, "    allowMtp=\"true\" \n");
		fprintf(f, "  /> \n");
	}
	fprintf(f, "</StorageList>\n");
	fclose(f);
	return 0;
}

int get_voldmanaged(char * buf, int idx)
{
	struct storage_item * s = &storage_list[idx];
	buf[0] = 0;
	strcat(buf, mnt_point[idx].name);
	strcat(buf, ":");
	if (s->part_num == PATR_NUM_AUTO) {
		strcat(buf, "auto");
	} else {
		sprintf(buf + strlen(buf), "%d", s->part_num);
	}
	if (!s->emulated)
		strcat(buf, ",noemulatedsd");
	if (!s->removable)
		strcat(buf, ",nonremovable");
	return 0;
}

int str_replace(char * buf, char prefix, int num, const char * text)
{
	char * p;
	char ss[16];
	size_t buflen, sslen, tlen, plen;

	buflen = strlen(buf);
	tlen = strlen(text);
	sslen = sprintf(ss, "%c%d", prefix, num);
	p = strstr(buf, ss);
	if (!p)
		return -1;
	plen = (size_t)(p - buf);
	memmove(p + tlen, p + sslen, buflen - plen - sslen);
	memcpy(p, text, tlen);
	return 0;
}

int create_ftab_storage()
{
	size_t i, sz;
	FILE * f;
	char vm[128];
	char tmp[2*1024] = {0};

	f = fopen(FILE_FSTAB_TEMPLATE, "r");
	if (f == NULL) {
		ERROR("can not open '%s', err: %s\n", FILE_FSTAB_TEMPLATE, strerror(errno));
		return -1;
	}
	if (fread(tmp, 1, sizeof(tmp)-1, f) == 0) {
		ERROR("can not read '%s'\n", FILE_FSTAB_TEMPLATE);
		fclose(f);
		return -2;
	}
	fclose(f);

	for (i = 0; i < ARRAY_SIZE(storage_list); i++) {
		if (i <= STOR_SECONDARY) {
			get_voldmanaged(vm, i);
			str_replace(tmp, '%', storage_list[i].sdcc, vm);
			ERROR("%s: msm_sdcc.%d voldmanaged=%s\n", __func__, storage_list[i].sdcc, vm);
		}
	}

	f = fopen(FILE_FSTAB, "a");
	if (f == NULL) {
		ERROR("can not open '%s', err: %s\n", FILE_FSTAB, strerror(errno));
		return -10;
	}
	fputs(tmp, f);
	fclose(f);

	return 0;
}

int get_part_num(int sdcc, const char * part_name)
{
	int rc;
	ssize_t len;
	char buf[256];
	char dev_name[256];
	char * p;

	sprintf(dev_name, "/dev/block/platform/msm_sdcc.%d/by-name/%s", sdcc, part_name);
	rc = access(dev_name, F_OK);
	if (rc) {
		ERROR("Partition '%s' NOT present on msm_sdcc.%d (err = %d) \n", part_name, sdcc, rc);
		return -1;
	}
	len = readlink(dev_name, buf, sizeof(buf)-1);
	if (len <= 0) {
		ERROR("readlink(%s) return err = %d \n", dev_name, (int)len);
		return -2;
	}
	buf[len] = 0;
	if (!strstr(buf, "/mmcblk"))
		return -3;
	p = strrchr(buf, 'p');
	if (!p)
		return -4;
	rc = strtol(p + 1, NULL, 10);
	if (rc <= 0)
		return -5;
	return rc;
}

void init_msm_properties(unsigned long msm_id, unsigned long msm_ver, char * board_type)
{
	UNUSED(msm_id);
	UNUSED(msm_ver);
	UNUSED(board_type);

	int rc;
	char value[PROP_VALUE_MAX];

	init_prop_forced_list();

	mount("rootfs", "/", "rootfs", MS_REMOUNT|0, NULL);
	ERROR("%s: rootfs remouted\n", __func__);

	rc = property_get("ro.boot.llcon", value);
	if (rc > 0) {
		if (value[0] != '0') {
			property_set("debug.sf.nobootanimation", "1");
			ERROR("%s: set 1 to debug.sf.nobootanimation\n", __func__);
		}
	}

	rc = property_get(PERSISTENT_PROPERTY_PLANNED_SWAP, value);
	if (rc && atoi(value))
		stor_swapped = 1;
	ERROR("%s: %s = %d\n", __func__, PERSISTENT_PROPERTY_PLANNED_SWAP, stor_swapped);

	rc = property_get(RO_PROPERTY_SWAP_SDCC, value);
	if (rc) {
		swap_sdcc = atoi(value);
		if (swap_sdcc < 0 || swap_sdcc > 2)
			swap_sdcc = 0;
	}
	ERROR("%s: %s = %d\n", __func__, RO_PROPERTY_SWAP_SDCC, swap_sdcc);

	if (swap_sdcc > 0) {
		storage_list[STOR_PRIMARY].emmc = 0;
		storage_list[STOR_PRIMARY].sdcc = 1;
		storage_list[STOR_PRIMARY].removable = 0;

		storage_list[STOR_SECONDARY].emmc = 1;
		storage_list[STOR_SECONDARY].sdcc = 2;
		storage_list[STOR_SECONDARY].removable = 0;
	}

	if (swap_sdcc) {
		struct storage_item x = storage_list[STOR_PRIMARY];
		storage_list[STOR_PRIMARY] = storage_list[STOR_SECONDARY];
		storage_list[STOR_SECONDARY] = x;
	}

	if (stor_swapped) {
		struct storage_item x = storage_list[STOR_PRIMARY];
		storage_list[STOR_PRIMARY] = storage_list[STOR_SECONDARY];
		storage_list[STOR_SECONDARY] = x;
		property_set(PERSISTENT_PROPERTY_SWAPPED, "1");
	} else {
		property_set(PERSISTENT_PROPERTY_SWAPPED, "0");
	}

	storage_list[STOR_PRIMARY].removable = 0;

	if (!storage_list[STOR_PRIMARY].emmc && !storage_list[STOR_SECONDARY].emmc) {
		storage_list[STOR_PRIMARY].storageDescription = STR_STORAGE_INTERNAL;
		storage_list[STOR_SECONDARY].storageDescription = STR_STORAGE_SDCARD;
	} else
	if (storage_list[STOR_PRIMARY].emmc) {
		storage_list[STOR_PRIMARY].storageDescription = STR_STORAGE_INTERNAL;
		storage_list[STOR_SECONDARY].storageDescription = STR_STORAGE_SDCARD;
	} else
	if (storage_list[STOR_SECONDARY].emmc) {
		storage_list[STOR_PRIMARY].storageDescription = STR_STORAGE_SDCARD;
		storage_list[STOR_SECONDARY].storageDescription = STR_STORAGE_INTERNAL;
	}

	rc = get_part_num(storage_list[STOR_PRIMARY].sdcc, "usbmsc");
	if (rc > 0)
		storage_list[STOR_PRIMARY].part_num = rc;

	rc = get_part_num(storage_list[STOR_SECONDARY].sdcc, "usbmsc");
	if (rc > 0)
		storage_list[STOR_SECONDARY].part_num = rc;

	create_ftab_storage();
	unlink(FILE_FSTAB_TEMPLATE);

	create_storage_list();

	mount("rootfs", "/", "rootfs", MS_REMOUNT|MS_RDONLY, NULL);
	ERROR("%s: rootfs remouted (RO)\n", __func__);
}
