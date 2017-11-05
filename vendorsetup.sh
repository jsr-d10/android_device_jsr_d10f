echo Sourcing vendor/qcom/proprietary/common/vendorsetup.sh
. vendor/qcom/proprietary/common/vendorsetup.sh

add_lunch_combo lineage_d10f-eng
add_lunch_combo lineage_d10f-user
add_lunch_combo lineage_d10f-userdebug

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

[ -d prebuilts/qemu-kernel/arm/ ] || mkdir -p prebuilts/qemu-kernel/arm/
[ -f prebuilts/qemu-kernel/arm/LINUX_KERNEL_COPYING ] || touch prebuilts/qemu-kernel/arm/LINUX_KERNEL_COPYING

[ -d vendor/cm/proprietary/ ] || mkdir vendor/cm/proprietary/
[ -f vendor/cm/proprietary/Term.apk ] || wget https://jackpal.github.com/Android-Terminal-Emulator/downloads/Term.apk -O vendor/cm/proprietary/Term.apk
[ -d vendor/cm/proprietary/lib/ ] || unzip -d vendor/cm/proprietary/ vendor/cm/proprietary/Term.apk lib/*

# Check and set correct javac, javap and java versions
javac="$(readlink /etc/alternatives/javac)"
if [ "$javac" != "/usr/lib/jvm/java-7-openjdk-amd64/bin/javac" ]; then
  echo Updating javac alternative to java-7-openjdk
  sudo update-alternatives --set javac /usr/lib/jvm/java-7-openjdk-amd64/bin/javac
fi
javap="$(readlink /etc/alternatives/javap)"
if [ "$javap" != "/usr/lib/jvm/java-7-openjdk-amd64/bin/javap" ]; then
  echo Updating javap alternative to java-7-openjdk
  sudo update-alternatives --set javap /usr/lib/jvm/java-7-openjdk-amd64/bin/javap
fi
java="$(readlink /etc/alternatives/java)"
if [ "$java" != "/usr/lib/jvm/java-7-openjdk-amd64/jre/bin/java" ]; then
  echo Updating java alternative to java-7-openjdk
  sudo update-alternatives --set java /usr/lib/jvm/java-7-openjdk-amd64/jre/bin/java
fi
