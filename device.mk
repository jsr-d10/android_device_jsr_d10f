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
#

$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)

#$(call inherit-product, vendor/jsr/d10f/d10f-vendor.mk)

# Overlay
DEVICE_PACKAGE_OVERLAYS += $(LOCAL_PATH)/overlay

# System Properties
#-include $(LOCAL_PATH)/system_prop.mk

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += persist.sys.usb.config=mtp
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += persist.service.adb.enable=1
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.adb.secure=0
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.secure=0
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.allow.mock.location=1
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.debuggable=1

# Screen density
PRODUCT_AAPT_CONFIG := normal hdpi xhdpi
PRODUCT_AAPT_PREF_CONFIG := xhdpi

PRODUCT_TAGS += dalvik.gc.type-precise

$(call inherit-product, frameworks/native/build/phone-xhdpi-1024-dalvik-heap.mk)

# init.d
PRODUCT_COPY_FILES += $(LOCAL_PATH)/etc/init.d/80tzdata_updater:/system/etc/init.d/80tzdata_updater

# ANT+
#PRODUCT_PACKAGES += \
#    AntHalService \
#    com.dsi.ant.antradio_library \
#    libantradio

# Audio
PRODUCT_PACKAGES += \
    audiod \
    audio.primary.msm8226 \
    audio_policy.msm8226 \
    audio.a2dp.default \
    audio.r_submix.default \
    audio.usb.default

PRODUCT_PACKAGES += \
    libaudio-resampler \
    libqcompostprocbundle \
    libqcomvisualizer \
    libqcomvoiceprocessing \
    tinymix

# Audio configuration
PRODUCT_COPY_FILES += $(LOCAL_PATH)/configs/audio_effects.conf:system/etc/audio_effects.conf
#PRODUCT_COPY_FILES += $(LOCAL_PATH)/configs/audio_platform_info.xml:system/etc/audio_platform_info.xml
PRODUCT_COPY_FILES += $(LOCAL_PATH)/configs/audio_policy.conf:system/etc/audio_policy.conf
PRODUCT_COPY_FILES += $(LOCAL_PATH)/configs/mixer_paths.xml:system/etc/mixer_paths.xml

# Bluetooth
PRODUCT_PROPERTY_OVERRIDES += bluetooth.hfp.client=1
PRODUCT_COPY_FILES += $(LOCAL_PATH)/rootdir/etc/init.qcom.bt.sh:system/etc/init.qcom.bt.sh

# Camera
PRODUCT_PACKAGES += libxml2
ifneq ($(QCPATH),)
PRODUCT_PACKAGES += camera.msm8226
endif

# Charger
PRODUCT_PACKAGES += \
    charger \
    charger_res_images

# CRDA
PRODUCT_PACKAGES += \
    crda \
    linville.key.pub.pem \
    regdbdump \
    regulatory.bin

# Display
PRODUCT_PACKAGES += \
    copybit.msm8226 \
    gralloc.msm8226 \
    hwcomposer.msm8226 \
    memtrack.msm8226 \
    liboverlay

# Ebtables
PRODUCT_PACKAGES += \
    ebtables \
    ethertypes \
    libebtc

# Filesystem
PRODUCT_PACKAGES += \
    e2fsck \
    fibmap.f2fs \
    fsck.f2fs \
    make_ext4fs \
    mkfs.f2fs \
    resize2fs \
    setup_fs

# FM
PRODUCT_PACKAGES += \
    FM2 \
    FMRecord \
    libqcomfm_jni \
    qcom.fmradio

PRODUCT_COPY_FILES += $(LOCAL_PATH)/rootdir/etc/init.qcom.fm.sh:system/etc/init.qcom.fm.sh

# GPS
PRODUCT_PACKAGES += gps.msm8226
PRODUCT_PACKAGES += libgps.utils
PRODUCT_PACKAGES += libloc_eng
PRODUCT_PACKAGES += gps.conf
ifeq ($(QCPATH),)
PRODUCT_PACKAGES += libloc_adapter
PRODUCT_PACKAGES += sap.conf
else
PRODUCT_PACKAGES += libloc_core
PRODUCT_PACKAGES += libloc_api_v02
endif

PRODUCT_COPY_FILES += $(LOCAL_PATH)/configs/gps.conf:system/etc/gps.conf

# IRSC
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/sec_config:system/etc/sec_config

# Keylayouts
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/keylayout/ft5x06_ts.kl:system/usr/keylayout/ft5x06_ts.kl \
    $(LOCAL_PATH)/keylayout/gpio-keys.kl:system/usr/keylayout/gpio-keys.kl \
    $(LOCAL_PATH)/keylayout/msm8226-tapan-snd-card_Button_Jack.kl:system/usr/keylayout/msm8226-tapan-snd-card_Button_Jack.kl \
    $(LOCAL_PATH)/keylayout/synaptics_rmi4_i2c.kl:system/usr/keylayout/synaptics_rmi4_i2c.kl

# Keystore
PRODUCT_PACKAGES += \
    keystore.msm8226

# Lights
PRODUCT_PACKAGES += \
    lights.msm8226

# MMC Utils
PRODUCT_PACKAGES += mmc

# ViewMem 
PRODUCT_PACKAGES += viewmem
    
# KeyHandler
PRODUCT_PACKAGES += \
    com.cyanogenmod.keyhandler

# Media
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/media_codecs.xml:system/etc/media_codecs.xml \
    $(LOCAL_PATH)/configs/media_profiles.xml:system/etc/media_profiles.xml


# OMX
PRODUCT_PACKAGES += \
    libc2dcolorconvert \
    libdashplayer \
    libdivxdrmdecrypt \
    libOmxAacEnc \
    libOmxAmrEnc \
    libOmxCore \
    libOmxEvrcEnc \
    libOmxQcelp13Enc \
    libOmxVdec \
    libOmxVdecHevc \
    libOmxVenc \
    libstagefrighthw \
    qcmediaplayer

PRODUCT_BOOT_JARS += \
    qcmediaplayer

# This build information
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/buildinfo/build-manifest.xml:system/etc/build-manifest.xml \
    $(LOCAL_PATH)/buildinfo/CHANGES.txt:system/etc/CHANGES.txt \
    $(LOCAL_PATH)/buildinfo/repo_state.txt:system/etc/repo_state.txt

# Permissions
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.audio.low_latency.xml:system/etc/permissions/android.hardware.audio.low_latency.xml \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:system/etc/permissions/android.hardware.bluetooth_le.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.telephony.cdma.xml:system/etc/permissions/android.hardware.telephony.cdma.xml \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
    frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml

# Power
PRODUCT_PACKAGES += \
    power.msm8226

# QRNG
PRODUCT_PACKAGES += \
    qrngd \
    qrngp

# Custom APK
PRODUCT_PACKAGES += JSR_Settings

# OpenDelta
PRODUCT_PACKAGES += OpenDelta

# Ramdisk
#PRODUCT_PACKAGES += chargeonlymode
PRODUCT_PACKAGES += fstab.d10f
PRODUCT_PACKAGES += fstab.template
PRODUCT_PACKAGES += init.qcom.rc
PRODUCT_PACKAGES += init.qcom.usb.rc
PRODUCT_PACKAGES += ueventd.qcom.rc
PRODUCT_PACKAGES += init.qcom.bt.sh
PRODUCT_PACKAGES += init.qcom.fm.sh
PRODUCT_PACKAGES += init.qcom.usb.sh
PRODUCT_PACKAGES += init.qcom.sensors.sh
PRODUCT_PACKAGES += init.qcom.gps.sh

# Recovery
PRODUCT_PROPERTY_OVERRIDES += ro.cwm.enable_key_repeat=true
PRODUCT_PROPERTY_OVERRIDES += ro.cwm.forbid_mount=/persist,/firmware
PRODUCT_PROPERTY_OVERRIDES += ro.cwm.forbid_format=/fsg,/firmware,/boot,/persist

# Sensors
PRODUCT_COPY_FILES += $(LOCAL_PATH)/configs/sensor_def_qcomdev.conf:system/etc/sensor_def_qcomdev.conf

# Thermal
#PRODUCT_PACKAGES += thermal-engine-8226.conf
PRODUCT_COPY_FILES += $(LOCAL_PATH)/configs/thermal-engine-8226.conf:system/etc/thermal-engine-8226.conf

# Torch
PRODUCT_PACKAGES += \
    Torch

# USB
PRODUCT_PACKAGES += com.android.future.usb.accessory
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += persist.sys.usb.config=mtp
PRODUCT_COPY_FILES += $(LOCAL_PATH)/rootdir/etc/init.qcom.usb.sh:system/etc/init.qcom.usb.sh

# UTC date
PRODUCT_BUILD_PROP_OVERRIDES += BUILD_UTC_DATE=0

# Wifi
PRODUCT_PACKAGES += \
    hostapd.accept \
    hostapd.deny \
    hostapd_default.conf \
    p2p_supplicant_overlay.conf \
    wpa_supplicant_overlay.conf \
    WCNSS_cfg.dat \
    WCNSS_qcom_cfg.ini \
    WCNSS_qcom_wlan_nv.bin

PRODUCT_PACKAGES += \
    libcurl \
    libqsap_sdk \
    libQWiFiSoftApCfg \
    wcnss_service

PRODUCT_PROPERTY_OVERRIDES += persist.debug.wfd.enable=1
PRODUCT_PROPERTY_OVERRIDES += persist.sys.wfd.virtual=0
PRODUCT_PROPERTY_OVERRIDES += persist.sys.ssr.restart_level=3
PRODUCT_PROPERTY_OVERRIDES += wifi.interface=wlan0

# MMI
#PRODUCT_COPY_FILES += \
#  $(LOCAL_PATH)/configs/mmi.cfg:system/etc/mmi.cfg \
#  $(LOCAL_PATH)/configs/qcom.cfg:system/etc/qcom.cfg \
#  $(LOCAL_PATH)/configs/sensor_def_qcomdev.conf:system/etc/sensor_def_qcomdev.conf \
#  $(LOCAL_PATH)/configs/vold.fstab:system/etc/vold.fstab \
#  $(LOCAL_PATH)/configs/ftm_test_config:system/etc/ftm_test_config \
#  $(LOCAL_PATH)/configs/izat.conf:system/etc/izat.conf \
#  $(LOCAL_PATH)/configs/lowi.conf:system/etc/lowi.conf \
#  $(LOCAL_PATH)/configs/quipc.conf:system/etc/quipc.conf \
#  $(LOCAL_PATH)/configs/gps.conf:system/etc/gps.conf \
#  $(LOCAL_PATH)/configs/qlog-conf.xml:system/etc/qlog-conf.xml 
