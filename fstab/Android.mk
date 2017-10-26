LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE           := fstab
LOCAL_MODULE_TAGS      := optional
LOCAL_SRC_FILES        := fstab.c log.c configuration.c
LOCAL_CFLAGS           += -Wall -Wextra
LOCAL_STATIC_LIBRARIES := libcutils libbase libc
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_SBIN_UNSTRIPPED)
include $(BUILD_EXECUTABLE)
