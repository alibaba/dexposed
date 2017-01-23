LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    memory.cpp

LOCAL_MODULE := libdexposed_art
LOCAL_MODULE_TAGS := optional

LOCAL_LDLIBS += -llog

LOCAL_CFLAGS := -DANDROID_NDK -std=gnu++11 -fexceptions -fpermissive

ifeq ($(TARGET_ARCH_ABI),armeabi)
    LOCAL_CFLAGS += -DARMEABI
endif

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_CFLAGS += -DARME64_V8A
endif

include $(BUILD_SHARED_LIBRARY)
