#
# Copyright (C) 2014 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Inherit from the proprietary version
-include vendor/jsr/d10f/BoardConfigVendor.mk

LOCAL_PATH := device/jsr/d10f

BOARD_VENDOR := jsr-qcom

TARGET_SPECIFIC_HEADER_PATH := $(LOCAL_PATH)/include

TARGET_GCC_VERSION_EXP := 4.8
# BOARD_USE_LEGACY_TOUCHSCREEN := true
DISABLE_DEXPREOPT := true

# Assert
TARGET_OTA_ASSERT_DEVICE := d10f,D10F,D10A_ROM,D10A_NEW_ROM,D10A_ROM_D10F,D10A_FGN,D10A_8G2G,D10A_HighScreen,Pentagram,D10A_Pentagram,BRAVA,D10CF_BRAVA,THAI_IMOBILE,D10A_THA_IMobile

# Platform
TARGET_BOARD_PLATFORM := msm8226
TARGET_BOARD_PLATFORM_GPU := qcom-adreno305

# Architecture
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_SMP := true
TARGET_CPU_VARIANT := krait

# Bootloader
TARGET_BOOTLOADER_BOARD_NAME := MSM8226
TARGET_NO_BOOTLOADER := true
TARGET_NO_RADIOIMAGE := true

# GCC BUILD FLAGS (only kernel)
#TARGET_USE_O3 := true
#OPT_MEMORY := true
STRICT_ALIASING := false
SUPPRESS_UNUSED_WARNING := true

# Flags
#TARGET_GLOBAL_CFLAGS += -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=softfp
#TARGET_GLOBAL_CPPFLAGS += -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=softfp

# Kernel
BOARD_KERNEL_CMDLINE := console=ttyHSL0,115200,n8 androidboot.console=ttyHSL0
BOARD_KERNEL_CMDLINE += androidboot.hardware=qcom androidboot.selinux=permissive
BOARD_KERNEL_CMDLINE += msm_rtb.filter=0x37
BOARD_KERNEL_CMDLINE += user_debug=31 debug ignore_loglevel
BOARD_KERNEL_CMDLINE += pmemlog=9 panic_restart=4

BOARD_KERNEL_BASE := 0x00000000
BOARD_KERNEL_PAGESIZE := 2048
BOARD_KERNEL_SEPARATED_DT := true
TARGET_KERNEL_SOURCE := kernel/jsr/msm8226
TARGET_KERNEL_CONFIG := jsr_d10f_defconfig 
BOARD_CUSTOM_BOOTIMG_MK := $(LOCAL_PATH)/mkbootimg.mk
BOARD_MKBOOTIMG_ARGS := --ramdisk_offset 0x02000000 --tags_offset 0x01e00000

# Init
TARGET_INIT_VENDOR_LIB := libinit_msm
TARGET_LIBINIT_DEFINES_FILE := $(LOCAL_PATH)/init/init_d10f.c
# TARGET_UNIFIED_DEVICE := true

# ANT+
#BOARD_ANT_WIRELESS_DEVICE := "vfs-prerelease"

# Audio
AUDIO_FEATURE_DISABLED_ANC_HEADSET := true
AUDIO_FEATURE_DISABLED_DS1_DOLBY_DDP := true
AUDIO_FEATURE_DISABLED_SSR := true
BOARD_USES_ALSA_AUDIO := true
TARGET_QCOM_AUDIO_VARIANT := caf
TARGET_AUDIO_HAL_PATH := hardware/qcom/audio-caf

# RIL
BOARD_PROVIDES_LIBRIL := true
COMMON_GLOBAL_CFLAGS += -DRIL_SUPPORTS_SEEK
ifeq ($(QCPATH),)
COMMON_GLOBAL_CFLAGS += -DRIL_VARIANT_LEGACY
else
PROTOBUF_SUPPORTED := true
COMMON_GLOBAL_CFLAGS += -DRILJ_USE_CLA
endif

# Bionic
# TARGET_USE_QCOM_BIONIC_OPTIMIZATION := true

# Bluetooth
ifeq ($(QCPATH),)
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := $(LOCAL_PATH)/bluetooth
BOARD_HAVE_BLUETOOTH := true
endif
BOARD_HAVE_BLUETOOTH_QCOM := true
BLUETOOTH_HCI_USE_MCT := true

# Camera
#USE_DEVICE_SPECIFIC_CAMERA := true
#USE_CAMERA_STUB := true
TARGET_DISPLAY_INSECURE_MM_HEAP := true
TARGET_CAMERA_HAL_PATH := hardware/qcom/camera

# Classpath
#PRODUCT_BOOT_JARS := $(subst $(space),:,$(PRODUCT_BOOT_JARS))

# Enable suspend during charger mode
BOARD_CHARGER_ENABLE_SUSPEND := true

# FM
BOARD_HAVE_QCOM_FM := true

# GPS
BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE := $(TARGET_BOARD_PLATFORM)
TARGET_NO_RPC := true
TARGET_GPS_HAL_PATH := hardware/qcom/gps
TARGET_GPS_LOC_API_PATH := $(TARGET_GPS_HAL_PATH)/loc_api/loc_api_v02
ifneq ($(QCPATH),)
GPS_LOC_API_V02_ENABLED := true
FEATURE_GNSS_BIT_API := true
endif

# CMHW (Hardware tunables)
BOARD_HARDWARE_CLASS := $(LOCAL_PATH)/cmhw/

# Display
BOARD_EGL_CFG := $(LOCAL_PATH)/configs/egl.cfg
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3
TARGET_USES_C2D_COMPOSITION := true
TARGET_USES_ION := true
USE_OPENGL_RENDERER := true
TARGET_DISPLAY_USE_RETIRE_FENCE := true
OVERRIDE_RS_DRIVER := libRSDriver_adreno.so
TARGET_QCOM_DISPLAY_VARIANT := caf-new
TARGET_DISPLAY_HAL_PATH := hardware/qcom/display-caf-new
GLOBAL_DISPLAY_HAL_PATH := $(TOP)/$(TARGET_DISPLAY_HAL_PATH)

# Shader cache config options
# Maximum size of the GLES Shaders that can be cached for reuse.
# Increase the size if shaders of size greater than 12KB are used.
MAX_EGL_CACHE_KEY_SIZE := 12*1024

# Maximum GLES shader cache size for each app to store the compiled shader
# binaries. Decrease the size if RAM or Flash Storage size is a limitation
# of the device.
MAX_EGL_CACHE_SIZE := 2048*1024

# Encryption
TARGET_HW_DISK_ENCRYPTION := true

# Lights
TARGET_PROVIDES_LIBLIGHT := true

# Media
TARGET_ENABLE_QC_AV_ENHANCEMENTS := true
TARGET_QCOM_MEDIA_VARIANT := caf-new
TARGET_MEDIA_HAL_PATH := hardware/qcom/media-caf-new
GLOBAL_MEDIA_HAL_PATH := $(TOP)/$(TARGET_MEDIA_HAL_PATH)

# Motorola
# TARGET_USES_MOTOROLA_LOG := true

# Power
TARGET_POWERHAL_VARIANT := qcom

# Partitions
BOARD_BOOTIMAGE_PARTITION_SIZE := 16777216
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 16777216
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1048576000
BOARD_CACHEIMAGE_PARTITION_SIZE := 134217728
BOARD_USERDATAIMAGE_PARTITION_SIZE := 3145677824
BOARD_FLASH_BLOCK_SIZE := 131072
BOARD_HAS_LARGE_FILESYSTEM := true

# Qualcomm support
COMMON_GLOBAL_CFLAGS += -DQCOM_BSP
COMMON_GLOBAL_CFLAGS += -DQCOM_HARDWARE
BOARD_USES_QCOM_HARDWARE := true
TARGET_USES_QCOM_BSP := true
# QCOM_BSP_WITH_GENLOCK := true
# COMMON_GLOBAL_CFLAGS += -DQCOM_BSP_ABI_HACK
# COMMON_GLOBAL_CFLAGS += -DUSE_MDP3

# Recovery
# TARGET_NO_RECOVERY := true
TARGET_NO_SEPARATE_RECOVERY := false
RECOVERY_VARIANT := cm
BOARD_HAS_NO_SELECT_BUTTON := true
BOARD_RECOVERY_SWIPE := true
BOARD_SUPPRESS_EMMC_WIPE := true
# BOARD_USE_CUSTOM_RECOVERY_FONT := \"roboto_15x24.h\"
TARGET_RECOVERY_PIXEL_FORMAT := "RGBX_8888"
TARGET_USERIMAGES_USE_EXT4 := true
# TARGET_USERIMAGES_USE_F2FS := true
TARGET_RECOVERY_FSTAB := $(LOCAL_PATH)/recovery/recovery.fstab

# TWRP
# HAVE_SELINUX := true
# DEVICE_RESOLUTION := 720x1280
# TW_IGNORE_MAJOR_AXIS_0 := true
# TW_INTERNAL_STORAGE_PATH := "/storage/sdcard1"
# TW_INTERNAL_STORAGE_MOUNT_POINT := "sdcard1"
# TW_EXTERNAL_STORAGE_PATH := "/storage/sdcard0"
# TW_EXTERNAL_STORAGE_MOUNT_POINT := "sdcard0"

# Time services
BOARD_USES_QC_TIME_SERVICES := true

# Vold
BOARD_VOLD_DISC_HAS_MULTIPLE_MAJORS := true
BOARD_VOLD_EMMC_SHARES_DEV_MAJOR := true
BOARD_VOLD_MAX_PARTITIONS := 40
TARGET_USE_CUSTOM_LUN_FILE_PATH := /sys/devices/platform/msm_hsusb/gadget/lun%d/file

# Wifi
BOARD_HAS_QCOM_WLAN := true
BOARD_WLAN_DEVICE := qcwcn
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_qcwcn
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_qcwcn
WIFI_DRIVER_MODULE_PATH := "/system/lib/modules/wlan.ko"
WIFI_DRIVER_MODULE_NAME := "wlan"
WPA_SUPPLICANT_VERSION := VER_0_8_X
WIFI_DRIVER_FW_PATH_STA := "sta"
WIFI_DRIVER_FW_PATH_AP := "ap"
WIFI_DRIVER_FW_PATH_P2P := "p2p"
TARGET_USES_WCNSS_CTRL := true
TARGET_USES_QCOM_WCNSS_QMI := true

# Developer's config
board_config_dev := \
	$(strip $(wildcard \
		$(SRC_TARGET_DIR)/board/$(TARGET_DEVICE)/BoardConfigDev.mk \
		$(shell test -d device && find device -maxdepth 4 -path '*/$(TARGET_DEVICE)/BoardConfigDev.mk') \
	))
ifeq ($(words $(board_config_dev)),1)
-include device/jsr/d10f/BoardConfigDev.mk
endif

