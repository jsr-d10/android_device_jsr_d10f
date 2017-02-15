#!/bin/sh

MYABSPATH=$(readlink -f "$0")
PATCHBASE=$(dirname "$MYABSPATH")

cd "$PATCHBASE"

mkdir -p overlay/packages/apps/Settings/res/xml/

xmlstarlet ed --pf --ps \
-u '/PreferenceScreen/PreferenceScreen[@android:key="additional_system_update_settings"]/@android:title' -v "@string/wifi_show_advanced" \
../../../packages/apps/Settings/res/xml/device_info_settings.xml > overlay/packages/apps/Settings/res/xml/device_info_settings.xml

mkdir -p overlay/packages/apps/CMFileManager/res/xml/
sed 's!%1\$s 1&gt; /dev/null \&amp;\&amp; /system/bin/stat -t %1!%1$s 2>\&amp;1 1\&gt; /dev/null | grep -Fv /.android_secure || /system/bin/stat -t %1!'  < ../../../packages/apps/CMFileManager/res/xml/command_list.xml > overlay/packages/apps/CMFileManager/res/xml/command_list.xml
