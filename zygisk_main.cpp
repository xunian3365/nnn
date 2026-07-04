#include <jni.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <unistd.h>
#include "zygisk.hpp"

using namespace zygisk;

typedef VkResult (*PFN_vkGetPhysicalDeviceProperties2)(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2* pProperties2);
static PFN_vkGetPhysicalDeviceProperties2 orig_vkGetPhysicalDeviceProperties2 = nullptr;

VkResult fake_vkGetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2* pProperties2) {
    VkResult result = orig_vkGetPhysicalDeviceProperties2(physicalDevice, pProperties2);

    if (result == VK_SUCCESS && pProperties2) {
        VkPhysicalDeviceProperties* pProps = &pProperties2->properties;
        pProps->vendorID = 0x13B5;
        pProps->deviceID = 0x9000;
        pProps->driverVersion = 0x00010000;
        strncpy(pProps->deviceName, "Maleoon 910", VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);

        VkBaseOutStructure* ext_structure = reinterpret_cast<VkBaseOutStructure*>(pProperties2->pNext);
        while (ext_structure != nullptr) {
            if (ext_structure->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES) {
                auto* driverProps = reinterpret_cast<VkPhysicalDeviceDriverProperties*>(ext_structure);
                driverProps->driverID = VK_DRIVER_ID_ARM_PROPRIETARY;
                strncpy(driverProps->driverName, "HiSilicon proprietary driver", VK_MAX_DESCRIPTION_SIZE);
                strncpy(driverProps->driverInfo, "Maleoon GPU Driver v1.0", VK_MAX_DESCRIPTION_SIZE);
            }
            if (ext_structure->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES) {
                auto* props11 = reinterpret_cast<VkPhysicalDeviceVulkan11Properties*>(ext_structure);
                memset(props11->deviceUUID, 0x7F, VK_UUID_SIZE);
            }
            ext_structure = ext_structure->pNext;
        }
    }
    return result;
}

class KirinSpoofModule : public Module {
public:
    void onLoad(Api* api, JNIEnv* env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs* args) override {
        // 模块禁用开关判断
        if (access("/data/adb/modules/kirin_snapdragon_ultra/disable", F_OK) == 0 ||
            access("/data/adb/modules/kirin_snapdragon_ultra/fake_cpuinfo", F_OK) != 0) {
            api->setOption(Option::FORCE_DENYLIST_UNHOOK);
            return;
        }

        // 兼容新旧 Zygisk SDK：args->package 替代不存在的 package_name
        const char* pkg_name = env->GetStringUTFChars(args->package, nullptr);
        bool target_app = false;

        if (pkg_name != nullptr) {
            if (strcmp(pkg_name, "com.tencent.tmgp.dfm") == 0) target_app = true;
            if (strcmp(pkg_name, "com.liuzh.deviceinfo") == 0) target_app = true;

            const char* proc_name = env->GetStringUTFChars(args->nice_name, nullptr);
            if (proc_name != nullptr) {
                if (strstr(proc_name, "com.tencent.tmgp.dfm:game")) {
                    target_app = true;
                }
                env->ReleaseStringUTFChars(args->nice_name, proc_name);
            }
        }

        if (target_app) {
            void* vulkan_handle = dlopen("libvulkan.so", RTLD_NOW);
            if (vulkan_handle) {
                auto orig = (PFN_vkGetPhysicalDeviceProperties2)dlsym(vulkan_handle, "vkGetPhysicalDeviceProperties2");
                if (orig) {
                    orig_vkGetPhysicalDeviceProperties2 = orig;
                    api->pltHookRegister("libvulkan.so", "vkGetPhysicalDeviceProperties2",
                                         (void*)fake_vkGetPhysicalDeviceProperties2, (void**)&orig_vkGetPhysicalDeviceProperties2);
                }
                dlclose(vulkan_handle);
            }
        }

        if (pkg_name != nullptr) {
            env->ReleaseStringUTFChars(args->package, pkg_name);
        }
    }

private:
    Api* api;
    JNIEnv* env;
};

// 注意此处分号，消除所有连锁编译错误
REGISTER_ZYGISK_MODULE(KirinSpoofModule);
