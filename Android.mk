LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := zygisk
LOCAL_SRC_FILES := zygisk_main.cpp
LOCAL_LDFLAGS += -llog -ldl
LOCAL_CFLAGS += -std=c++17 -Os
include $(BUILD_SHARED_LIBRARY)