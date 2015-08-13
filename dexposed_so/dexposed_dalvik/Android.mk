LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifneq ($(shell expr $(PLATFORM_SDK_VERSION) < 14), 0)
LOCAL_PRELINK_MODULE := false
endif

LOCAL_SRC_FILES:= dexposed.cpp

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
	libandroid_runtime \
	libdvm \
	libstlport \
	libdl

ifneq ($(shell expr $(PLATFORM_SDK_VERSION) \> 15), 0)
LOCAL_SHARED_LIBRARIES += libandroidfw
endif

LOCAL_C_INCLUDES += dalvik \
                    dalvik/vm \
                    external/stlport/stlport \
                    bionic \
                    bionic/libstdc++/include
LOCAL_CFLAGS += -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

ifeq ($(strip $(WITH_JIT)),true)
LOCAL_CFLAGS += -DWITH_JIT
endif


ifeq ($(strip $(XPOSED_SHOW_OFFSETS)),true)
LOCAL_CFLAGS += -DXPOSED_SHOW_OFFSETS
endif

LOCAL_MODULE:= libdexposed
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
