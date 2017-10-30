echo Sourcing vendor/qcom/proprietary/common/vendorsetup.sh
. vendor/qcom/proprietary/common/vendorsetup.sh

add_lunch_combo cm_d10f-eng
add_lunch_combo cm_d10f-user
add_lunch_combo cm_d10f-userdebug

sh device/jsr/d10f/patches/apply.sh "device/jsr/d10f/patches/"
croot

sh device/jsr/d10f/update-icu.sh
croot

sh device/jsr/d10f/update-overlay.sh
croot

#rm -f out/target/product/d10f/root/init.qcom.sdcard.rc
#rm -rf out/target/product/d10f/obj/ETC/init.qcom.sdcard.rc_intermediates
rm -rf out/target/product/d10f/obj/PACKAGING/target_files_intermediates
rm -f out/target/product/d10f/system/build.prop
rm -f out/target/product/d10f/root/default.prop
