LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := system/core/init
LOCAL_CFLAGS := -Wall -Wextra -Werror
LOCAL_SRC_FILES := init_d10f.cpp
LOCAL_MODULE := libinit_d10f
LOCAL_SHARED_LIBRARIES := libcutils libbase

include $(BUILD_STATIC_LIBRARY)
