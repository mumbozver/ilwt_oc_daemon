LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := ilwt_oc
LOCAL_SRC_FILES := ilwt_oc.c
LOCAL_LDLIBS := -llog
include $(BUILD_EXECUTABLE)
