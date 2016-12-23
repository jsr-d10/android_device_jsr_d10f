LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE           := bdaddr_jsr
LOCAL_MODULE_TAGS      := optional
LOCAL_SRC_FILES        := bdaddr_jsr.c
LOCAL_CFLAGS           += -Wall
LOCAL_SHARED_LIBRARIES := libc libcutils libutils liblog libqminvapi
include $(BUILD_EXECUTABLE)
