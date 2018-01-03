#include <stdlib.h>
#include <stdio.h>
#include "log.h"
#include "configuration.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <sys/mman.h>
#include <cutils/properties.h>
#include <string.h>

#include <sys/stat.h>
#define FALSE 0
#define TRUE 1

#define PERSISTENT_PROPERTY_DIR  "/data/property"
#define PERSISTENT_PROPERTY_CONFIGURATION_NAME "persist.storages.configuration"
#define STORAGES_CONFIGURATION_CLASSIC   "0"
#define STORAGES_CONFIGURATION_INVERTED  "1"
#define STORAGES_CONFIGURATION_DATAMEDIA "2"
#define BOOT_PROPERTY_SDCC_CONFIGURATION_NAME "ro.boot.swap_sdcc"
#define SDCC_CONFIGURATION_NORMAL   "0"
#define SDCC_CONFIGURATION_INVERTED "1"
#define SDCC_CONFIGURATION_ISOLATED "2"
#define SERVICE_VOLD "vold"
#define USBMSC_PATH "/dev/block/platform/msm_sdcc.1/by-name/usbmsc"

#define STORAGE_XML_PATH "/data/system/storage.xml"
#define STORAGE_XML_TMP_PATH "/data/system/storage.tmp"
#define STORAGE_XML_HEADER "<?xml version='1.0' encoding='utf-8' standalone='yes' ?>\n"
#define STORAGE_XML_VOLUMES_TOKEN "<volumes version=\""
#define STORAGE_XML_PRIMARY_PHYSICAL_UUID_TOKEN "primaryStorageUuid=\"primary_physical\" "

#define STORAGE_XML_CONFIG_CLASSIC_FMT   "<volumes version=\"%d\" primaryStorageUuid=\"primary_physical\" forceAdoptable=\"%s\">\n"
#define STORAGE_XML_CONFIG_DATAMEDIA_FMT "<volumes version=\"%d\" forceAdoptable=\"%s\">\n"

char *mmap_xml_configuration(off_t *size) {
	FILE *xml_config_filestream = fopen(STORAGE_XML_PATH, "r+");
	if (xml_config_filestream == NULL) {
		ERROR("Unable to mmap storage configuration XML \"%s\" - storages may be inconsistent (errno: %d (%s))!\n",
		      STORAGE_XML_PATH, errno, strerror(errno));
		return NULL;
	}

	fseeko(xml_config_filestream, 0l, SEEK_END);
	*size = ftello(xml_config_filestream)+strlen(STORAGE_XML_PRIMARY_PHYSICAL_UUID_TOKEN); // some space for expansion
	fseeko(xml_config_filestream, 0l, SEEK_SET);

	char *config = (char *) mmap(NULL, *size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fileno(xml_config_filestream), 0);
	if (config == MAP_FAILED) {
		ERROR("Unable to mmap storage configuration XML \"%s\" - storages may be inconsistent (errno: %d (%s))!\n", STORAGE_XML_PATH, errno, strerror(errno));
		return NULL;
	}
	fclose(xml_config_filestream); // mmaped data still available
	return config;
}

void update_xml_configuration(char *xml_config, off_t config_size, int isDatamedia) {
	char *volumes=NULL, *version=NULL;
	version = strstr(xml_config, STORAGE_XML_HEADER);
	volumes = strstr(xml_config, STORAGE_XML_VOLUMES_TOKEN);
	if (version == NULL || volumes == NULL) {
		ERROR("Storage config xml \"%s\" looks werid - erasing it\n", STORAGE_XML_PATH);
		unlink(STORAGE_XML_PATH);
		return;
	}

	int scanned_size=0, fields=0, iversion=0, buffer_size=0, printed_bytes, buffer_free_bytes=0;
	char primaryStorageUuid[64+1], forceAdoptable[64+1];
	char *data=NULL, *rest_of_config=NULL;

	buffer_size=config_size + strlen(STORAGE_XML_PRIMARY_PHYSICAL_UUID_TOKEN); // needed if migrating from datamedia to classic
	data = (char *) calloc(1, buffer_size);
	if (data == NULL) {
		ERROR("Out of memory while allocating %d bytes\n", buffer_size);
		return;
	}

	data += sprintf(data, STORAGE_XML_HEADER); // initialize config header

	if (isDatamedia) {
		// only datamedia needs emulated storage!
		fields = sscanf(volumes,
		                "<volumes version=\"%d\" forceAdoptable=\"%64[^\"]\">\n%n",
		                &iversion, forceAdoptable, &scanned_size);
		if (fields == 2) {
			free (data);
			return; // nothing to do here!
		}

		ERROR("Storage config xml \"%s\" needs updating (looks like classic)!\n", STORAGE_XML_PATH);
		fields = sscanf(volumes,
		                "<volumes version=\"%d\" primaryStorageUuid=\"%64[^\"]\" forceAdoptable=\"%64[^\"]\">\n%n",
		                &iversion, primaryStorageUuid, forceAdoptable, &scanned_size);
		if (fields != 3) {
			ERROR("Storage config xml \"%s\" is werid - got %d fields (want 3)!\n", STORAGE_XML_PATH, fields);
			free (data);
			return;
		}
		printed_bytes=sprintf(data, STORAGE_XML_CONFIG_DATAMEDIA_FMT, iversion, forceAdoptable);
		data += printed_bytes;
		buffer_free_bytes=buffer_size-strlen(STORAGE_XML_HEADER)-printed_bytes;
	} else {
		fields = sscanf(volumes,
		                "<volumes version=\"%d\" primaryStorageUuid=\"%64[^\"]\" forceAdoptable=\"%64[^\"]\">\n%n",
		                &iversion, primaryStorageUuid, forceAdoptable, &scanned_size);
		if (fields == 3) {
			free (data);
			return; // nothing to do here!
		}

		ERROR("Storage config xml \"%s\" needs updating (looks like datamedia)!\n", STORAGE_XML_PATH);
		fields = sscanf(volumes,
		                "<volumes version=\"%d\" forceAdoptable=\"%64[^\"]\">\n%n",
		                &iversion, forceAdoptable, &scanned_size);
		if (fields != 2) {
			ERROR("Storage config xml \"%s\" is werid - got %d fields (want 2)!\n", STORAGE_XML_PATH, fields);
			free (data);
			return; // nothing to do here!
		}
		buffer_free_bytes=buffer_size - strlen(STORAGE_XML_HEADER) - scanned_size
		                  - strlen (STORAGE_XML_PRIMARY_PHYSICAL_UUID_TOKEN) - 1;
		printed_bytes = sprintf(data, STORAGE_XML_CONFIG_CLASSIC_FMT, iversion, forceAdoptable);
		data += printed_bytes;
	}
	// just copy rest of config data
	rest_of_config=xml_config + strlen(STORAGE_XML_HEADER) + scanned_size;
	strncpy(data, rest_of_config, buffer_free_bytes);
	ERROR("Going to write '%s' into config %s\n", data, STORAGE_XML_PATH);
	ERROR("Updating %s\n", STORAGE_XML_PATH);
	munmap(xml_config, config_size);
	FILE *storage_config = fopen(STORAGE_XML_PATH, "w");
	if (storage_config == NULL) {
		ERROR("Unable to open %s for writing (errno=%d: %s)\n", STORAGE_XML_PATH, errno, strerror(errno));
		free(data);
		return;
	}
	fwrite(data, strlen(data)-1, 1, storage_config);
	fclose(storage_config);
	free (data);
}

void set_storage_props(void)
{
	char value[PROP_VALUE_MAX+1];
	int isDatamedia = FALSE;
	int rc = property_get(PERSISTENT_PROPERTY_CONFIGURATION_NAME, value, "");
	if (rc == 0) { // If the storages configuration property is unspecified
		ERROR("Storages configuration is undefined (" PERSISTENT_PROPERTY_CONFIGURATION_NAME
		      " == %s), trying to guess best default value\n", value);
		if (access(USBMSC_PATH, F_OK) == 0) { // Check for usbmsc partition in primary storage
			rc = property_get(BOOT_PROPERTY_SDCC_CONFIGURATION_NAME, value, "");
			if (rc && !strcmp(value, SDCC_CONFIGURATION_NORMAL)) { // usbmsc is present, so check SDCC config
				strncpy(value, STORAGES_CONFIGURATION_CLASSIC, PROP_VALUE_MAX);
			} else {
				strncpy(value, STORAGES_CONFIGURATION_INVERTED, PROP_VALUE_MAX);
			}
		} else { // usbmsc is not present, so default storages configuration will always be datamedia
				strncpy(value, STORAGES_CONFIGURATION_DATAMEDIA, PROP_VALUE_MAX);
		}
		rc = strlen(value);
		property_set(PERSISTENT_PROPERTY_CONFIGURATION_NAME, value);
	}

	if (rc && !strcmp(value, STORAGES_CONFIGURATION_DATAMEDIA)) { // if datamedia
		ERROR("Got datamedia storage configuration (" PERSISTENT_PROPERTY_CONFIGURATION_NAME " == %s)\n", value);
		isDatamedia = TRUE;
	} else if (rc && !strcmp(value, STORAGES_CONFIGURATION_INVERTED)) { // if swapped
		ERROR("Got inverted storage configuration (" PERSISTENT_PROPERTY_CONFIGURATION_NAME " == %s)\n", value);
		property_set("ro.vold.primary_physical", "1");
	} else { // if classic
		ERROR("Got classic storage configuration (" PERSISTENT_PROPERTY_CONFIGURATION_NAME " == %s)\n", value);
		property_set("ro.vold.primary_physical", "1");
	}

        off_t size;
        char *xml_config=mmap_xml_configuration(&size);
	if (xml_config) {
		update_xml_configuration(xml_config, size, isDatamedia);
		munmap(xml_config, size);
	}

	ERROR("Storages configuration applied\n");
}
