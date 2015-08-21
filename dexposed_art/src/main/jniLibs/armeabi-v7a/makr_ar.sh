#!/bin/bash

/server/android-ndk-r10e/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-g++ \
  --sysroot=/server/android-ndk-r10e/platforms/android-21/arch-arm \
  -I../../jni/include/art/runtime \
  -I../../jni/include/art/runtime/arch/arm \
  ../../jni/art_quick_dexposed_invoke_handler.S -o libart_quick_dexposed_invoke_handler.so -c
