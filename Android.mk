LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# 输出产物：libzygisk.so
LOCAL_MODULE := zygisk

# 源码文件
LOCAL_SRC_FILES := zygisk_main.cpp

# C++编译参数：C++20标准，关闭异常/RTTI，精简体积
LOCAL_CPPFLAGS += -std=c++20 -fno-exceptions -fno-rtti -Wall

# 系统依赖：日志库、动态链接库，Vulkan运行时动态dlopen无需静态链接
LOCAL_LDLIBS += -llog -ldl

# 头文件检索路径
LOCAL_C_INCLUDES += $(LOCAL_PATH)

# 生成动态共享库
include $(BUILD_SHARED_LIBRARY)
