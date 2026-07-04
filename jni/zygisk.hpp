#pragma once
#include <jni.h>
#include <sys/types.h>

namespace zygisk {

enum class Option : int {
    FORCE_DENYLIST_UNHOOK = 0,
};

struct AppSpecializeArgs {
    jint uid;
    jint gid;
    jintArray gids;
    jint runtime_flags;
    jint mount_external;
    jstring package_name;
    jstring nice_name;
    jboolean is_child_zygote;
    jboolean is_first_boot;
};

class Api {
public:
    virtual ~Api() = default;
    virtual void setOption(Option opt) = 0;
    virtual void pltHookRegister(const char* lib, const char* symbol, void* fn, void** orig) = 0;
    virtual void pltHookCommit() = 0;
};

class Module {
public:
    virtual ~Module() = default;
    virtual void onLoad(Api* api, JNIEnv* env) = 0;
    virtual void preAppSpecialize(AppSpecializeArgs* args) = 0;
    virtual void postAppSpecialize(const AppSpecializeArgs* args) {}
};

} // namespace zygisk

#define REGISTER_ZYGISK_MODULE(clazz) \
extern "C" __attribute__((visibility("default"))) \
zygisk::Module* zygisk_module_entry() { \
    return new clazz(); \
}
