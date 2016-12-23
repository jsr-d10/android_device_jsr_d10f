#
# Copyright (C) 2016 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Inherit from the common Open Source product configuration
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit from hardware-specific part of the product configuration
$(call inherit-product, device/jsr/d10f/device.mk)
$(call inherit-product-if-exists, vendor/jsr/d10f/d10f-vendor.mk)

# Inherit some common CM stuff
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

# Device identifier. This must come after all inclusions.
PRODUCT_DEVICE := d10f
PRODUCT_NAME := cm_d10f
PRODUCT_BRAND := JSR
PRODUCT_MODEL := D10F
PRODUCT_MANUFACTURER := JSR Tech

PRODUCT_GMS_CLIENTID_BASE := android-jsr

PRODUCT_BUILD_PROP_OVERRIDES += \
    PRODUCT_NAME=d10f
