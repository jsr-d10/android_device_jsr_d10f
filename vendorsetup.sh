build_root=$(pwd) # vendorsetup.sh is sourced by build/envsetup.sh in root of android build tree. Hope that nobody can correctly source it not from root of android tree.

echo Touching prebuilts/qemu-kernel/arm/LINUX_KERNEL_COPYING
mkdir -p prebuilts/qemu-kernel/arm/
touch prebuilts/qemu-kernel/arm/LINUX_KERNEL_COPYING

echo "======================= Applying patches: enum ======================="
patches_path="$build_root/device/jsr/d10f/patches/"

cd $patches_path

unset repos

for patch in `find -type f -name '*.patch'|cut -d / -f 2-|sort`; do
	cd $patches_path

	# Strip patch title to get git commit title - git ignore [text] prefixes in beginning of patch title during git am
	title=$(sed -rn "s/Subject: (\[[^]]+] *)*//p" "$patch")
	absolute_patch_path="$patches_path/$patch"
	
	# Supported both path/to/repo_with_underlines/file.patch and path_to_repo+with+underlines/file.patch (both leads to path/to/repo_with_underlines)
	repo_to_patch="$(if dirname $patch|grep -q /; then dirname $patch; else dirname $patch |tr '_' '/'|tr '+' '_'; fi)"

	echo -n "Is $repo_to_patch patched for '$title' ?.. "

        if [ ! -d $build_root/$repo_to_patch ] ; then
                echo "$repo_to_patch NOT EXIST! Go away and check your manifests. Skipping this patch."
                continue
        fi

	cd $build_root/$repo_to_patch

	if (git log |fgrep -qm1 "$title" ); then
		echo -n Yes
	  commit_hash=$(git log --oneline |fgrep -m1 "$title"|cut -d ' ' -f 1)
	  if [ q"$commit_hash" != "q" ]; then
		  commit_id=$(git format-patch -1 --stdout $commit_hash |git patch-id|cut -d ' ' -f 1)
		  patch_id=$(git patch-id < $absolute_patch_path|cut -d ' ' -f 1)
		  if [ "$commit_id" = "$patch_id" ]; then
			  echo ', patch matches'
		  else
			  echo ', PATCH MISMATCH!'
			  echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
			  sed '0,/^$/d' $absolute_patch_path|head -n -3  > /tmp/patch
			  git show --stat $commit_hash -p --pretty=format:%b > /tmp/commit
			  diff -u /tmp/patch /tmp/commit
			  rm /tmp/patch /tmp/commit
			  echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
			  echo ' Resetting branch!'
			  git checkout $commit_hash~1
			  git am $absolute_patch_path || git am --abort
		  fi
	else
		echo "Unable to get commit hash for '$title'! Something went wrong!"
		sleep 20
	fi
	else
		echo No
		echo "Trying to apply patch $(basename "$patch") to '$repo_to_patch'"
		if ! git am $absolute_patch_path; then
			echo "!!!!!!!!!!!! Failed, aborting git am !!!!!!!!!!!!!!"
			git am --abort
		fi
	fi
done
cd $build_root
echo "======================= Applying patches: finished ======================="

echo "Updating overlay"

sh device/jsr/d10f/update-overlay.sh
rm -f out/target/product/d10f/system/build.prop

export QCPATH=

add_lunch_combo cm_d10f-eng
add_lunch_combo cm_d10f-user
add_lunch_combo cm_d10f-userdebug
