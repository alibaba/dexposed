How to build libdexposed.so?
-----------
Dexposed framework has two components. One is java bridge(dexposedbridge.jar), other is native so(libdexposed.so/libdexposed_l.so).
This guide will show you how to build the native so.

Because of huge change from dalvik to art in AOSP, we split native source code into two folders.
One is for dalvik runtime which named "dexposed_dalvik", other is for art runtime which named "dexposed_art".
dexposed_dalvik folder will product "libdexposed.so" and other will product libdexposed_l.so.


Step 1:
-----------------
* Download the AOSP from the [guide](https://source.android.com/source/building.html) and build AOSP once at least.
this step will take a lot of times, and you may meet a lot of compile errors due to the environment.

NOTE: The AOSP's version must be consistent to the native code. If you want to build dalvik so, you should download the AOSP version less than 4.4.
If you want to build art so, you should download the AOSP version 5.0.

Step 2:
-----------
* If you success compile the AOSP code. You can build the dexposed native so. Build dexposed native so depend on some (dalvik/art) runtime code.

* Now we use dalvik as example.

* First copy dexposed_dalvik folder to ANDROID_SOURCE_CODE/frameworks/base/cmds.
* Second cd into ANDROID_SOURCE_CODE/frameworks/base/cmds
* Third do cmd 'mmm -B dexposed_dalvik'. Then you will see it will start compile.
* If compile success, you will see the so in ANDROID_SOURCE_CODE/out/target/product/generic/system/lib/

-----------

Thank you! God bless you!