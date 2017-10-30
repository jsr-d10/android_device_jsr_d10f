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

# Release name
PRODUCT_RELEASE_NAME := d10f

# Boot animation
TARGET_SCREEN_HEIGHT := 1280
TARGET_SCREEN_WIDTH := 720

# Inherit some common CM stuff.
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit from d10f device
$(call inherit-product, device/jsr/d10f/device.mk)
$(call inherit-product-if-exists, device/jsr/d10f/sepolicy.mk)
#$(call inherit-product-if-exists, vendor/cm/sepolicy/sepolicy.mk)
$(call inherit-product-if-exists, vendor/jsr/d10f/d10f-vendor.mk)

# Override bootanimation
#PRODUCT_COPY_FILES += \
#    $(LOCAL_PATH)/bootanimation.zip:system/media/bootanimation.zip

# Device identifier. This must come after all inclusions
PRODUCT_DEVICE := d10f
PRODUCT_NAME := lineage_d10f
PRODUCT_BRAND := JSR
PRODUCT_MODEL := D10F
PRODUCT_MANUFACTURER := JSR Tech

PRODUCT_GMS_CLIENTID_BASE := android-google
