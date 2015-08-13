LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	dexposed.cpp \
	art_quick_dexposed_invoke_handler.S

LOCAL_CFLAGS += -std=c++0x -O0
LOCAL_C_INCLUDES := \
	$(JNI_H_INCLUDE) \
	art/runtime/ \
	art/runtime/entrypoints/quick/ \
	dalvik/ \
	$(ART_C_INCLUDES) \
	external/gtest/include \
	external/valgrind/main/include \
	external/valgrind/main/ \

include external/libcxx/libcxx.mk


LOCAL_SHARED_LIBRARIES := libutils liblog libart libc++ libcutils

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libdexposed_l

include $(BUILD_SHARED_LIBRARY)
