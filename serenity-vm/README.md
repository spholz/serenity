# Building

1. Download the Android source: <https://source.android.com/docs/setup/download>
2. Copy this directory to frameworks/native/samples/serenity-vm.
3. Apply this patch:

```patch
diff --git i/libs/binder/Android.bp w/libs/binder/Android.bp
index a3499c1963..7bc523a922 100644
--- i/libs/binder/Android.bp
+++ w/libs/binder/Android.bp
@@ -923,6 +923,7 @@ cc_library {
     visibility: [
         ":__subpackages__",
         "//packages/modules/Virtualization:__subpackages__",
+        "//frameworks/native/samples:__subpackages__",
         "//device/google/cuttlefish/shared/minidroid:__subpackages__",
         "//visibility:any_system_partition",
     ],
```

4. Build it

```sh
cd $ANDROID_SOURCE_DIR/frameworks/native/samples/serenity-vm && ./build.sh
```

# Running

1. Build a SerenityOS UEFI disk image

```sh
cd $SERENITY_SOURCE_DIR
Meta/serenity.sh image aarch64
ninja -C Build/aarch64 uefi-image
```

2. Copy the disk image

```sh
cd $SERENITY_SOURCE_DIR
adb shell mkdir /data/local/tmp/serenity-vm
adb push Build/aarch64/uefi_disk_image /data/local/tmp/serenity-vm
```

3. Copy `serenity-vm` to the target

```sh
adb push $ANDROID_SOURCE_DIR/out/target/product/vsoc_arm64/system/bin/serenity-vm /tmp
```

4. Launch the VM

```sh
adb shell /tmp/serenity-vm
```

5. Show logs (optional)

```sh
adb logcat -v color -s virtmgr crosvm DEBUG
```
