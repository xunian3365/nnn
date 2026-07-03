/* SPDX-License-Identifier: MIT */

/* Zygisk API version 4 - Magisk v26.0+ */

#pragma once

#include <jni.h>

#define ZYGISK_API_VERSION 4

namespace zygisk {

struct AppSpecializeArgs {
    jint &uid;
    jint &gid;
    jintArray &gids;
    jint &runtime_flags;
    jobjectArray &rlimits;
    jint &mount_external;
    jstring &se_info;
    jstring &nice_name;
    jintArray &is_child_zygote;
    jstring &instruction_set;
    jstring &app_data_dir;
    jboolean &is_top_app;
    jobjectArray &pkg_data_info_list;
    jobjectArray &whitelisted_data_info_list;
    jboolean &mount_data_dirs;
    jboolean &use_legacy_data;
};

struct ServerSpecializeArgs {
    jint &uid;
    jint &gid;
    jintArray &gids;
    jint &runtime_flags;
    jobjectArray &rlimits;
    jlong &permitted_capabilities;
    jlong &effective_capabilities;
};

enum class Option : int {
    // 强制卸载/擦除当前进程中的所有 Zygisk 框架钩子和特征痕迹
    // 这对于防止游戏反作弊扫描内存特征（如 maps 扫描）至关重要
    FORCE_DENYLIST_UNHOOK = 0,
    DLCLOSE_MODULE_LIBRARY = 1,
};

class Api {
public:
    // 连接并注册一个 PLT Hook
    virtual void pltHookRegister(const char *regex, const char *symbol, void *new_func, void **old_func) = 0;

    // 排除或者隐藏特定的文件路径（对于旧版本兼容，高版本推荐直接使用内核层 SuSFS）
    virtual void excludeFileFromCompanion(const char *path) = 0;

    // 设置进程级别的特殊选项（如 FORCE_DENYLIST_UNHOOK）
    virtual void setOption(Option opt) = 0;

    // 获取当前系统的全局特征标志
    virtual int getFlags() = 0;

    // 获取当前进程与伴随进程（Companion Process）通信的通信管道套接字 FD
    virtual int getCompanionFd() = 0;
};

class ModuleBase {
public:
    virtual void onLoad(Api *api, JNIEnv *env) = 0;
    virtual void preAppSpecialize(AppSpecializeArgs *args) {}
    virtual void postAppSpecialize(const AppSpecializeArgs *args) {}
    virtual void preServerSpecialize(ServerSpecializeArgs *args) {}
    virtual void postServerSpecialize(const ServerSpecializeArgs *args) {}
};

} // namespace zygisk

// 用于向 Magisk/Zygisk 框架注册自定义模块类的宏定义
#define REGISTER_ZYGISK_MODULE(clazz) \
extern "C" __attribute__((visibility("default"))) \
void zygisk_module_entry(long api_version, void *Api, void *env) { \
    if (api_version < ZYGISK_API_VERSION) return; \
    auto api = reinterpret_cast<zygisk::Api *>(Api); \
    auto JNI = reinterpret_cast<JNIEnv *>(env); \
    auto module = new clazz(); \
    api->pltHookRegister(".*", "zygote_module_entry", reinterpret_cast<void *>(module), nullptr); \
    module->onLoad(api, JNI); \
}
