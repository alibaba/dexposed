#!/bin/bash

/server/android-ndk-r10e/toolchains/x86-4.9/prebuilt/darwin-x86_64/bin/i686-linux-android-g++ \
  --sysroot=/server/android-ndk-r10e/platforms/android-21/arch-x86 \
  -I../../jni/include/art/runtime \
  -I../../jni/include/art/runtime/arch/x86 \
  ../../jni/art_quick_dexposed_invoke_handler.S -o libart_quick_dexposed_invoke_handler.so -c
