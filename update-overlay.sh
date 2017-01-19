#!/bin/sh

MYABSPATH=$(readlink -f "$0")
PATCHBASE=$(dirname "$MYABSPATH")

cd "$PATCHBASE"

mkdir -p overlay/packages/apps/Settings/res/xml/

xmlstarlet ed --pf --ps \
-u '/PreferenceScreen/cyanogenmod.preference.SelfRemovingPreference[@android:key="cm_updates"]/@android:title'                -v "@string/system_update_settings_list_item_title" \
-u '/PreferenceScreen/cyanogenmod.preference.SelfRemovingPreference[@android:key="cm_updates"]/@cm:requiresPackage'           -v "eu.chainfire.opendelta" \
-u '/PreferenceScreen/cyanogenmod.preference.SelfRemovingPreference[@android:key="cm_updates"]/intent/@android:targetPackage' -v "eu.chainfire.opendelta" \
-u '/PreferenceScreen/cyanogenmod.preference.SelfRemovingPreference[@android:key="cm_updates"]/intent/@android:targetClass'   -v "eu.chainfire.opendelta.MainActivity" \
-u '/PreferenceScreen/PreferenceScreen[@android:key="additional_system_update_settings"]/@android:title'                      -v "@string/wifi_show_advanced" \
../../../packages/apps/Settings/res/xml/device_info_settings.xml > overlay/packages/apps/Settings/res/xml/device_info_settings.xml
