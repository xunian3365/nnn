LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := zygisk
LOCAL_SRC_FILES := zygisk_main.cpp
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra
LOCAL_LDLIBS += -llog -ldl
LOCAL_C_INCLUDES += $(LOCAL_PATH)

include $(BUILD_SHARED_LIBRARY)
