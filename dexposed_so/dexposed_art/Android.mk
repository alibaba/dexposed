LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	dexposed.cpp \
	art_quick_dexposed_invoke_handler.S

LOCAL_CFLAGS += -std=c++0x -O0 -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION) -Wno-unused-parameter 
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

ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 21)))
	ifdef ART_IMT_SIZE
	  LOCAL_CFLAGS += -DIMT_SIZE=$(ART_IMT_SIZE)
	else
	  # Default is 64
	  LOCAL_CFLAGS += -DIMT_SIZE=64
	endif
endif

LOCAL_SHARED_LIBRARIES := libutils liblog libart libc++ libcutils

LOCAL_MODULE_TAGS := optional

ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 21)))
	LOCAL_MODULE := libdexposed_l51
else
	LOCAL_MODULE := libdexposed_l
endif

include $(BUILD_SHARED_LIBRARY)
