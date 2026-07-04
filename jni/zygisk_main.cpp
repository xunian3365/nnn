#include <jni.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h> 
#include "zygisk.hpp"

using namespace zygisk;

// ========== 自研Vulkan最小依赖集 不依赖系统头文件 彻底消除版本冲突 ==========
#define VK_MAX_PHYSICAL_DEVICE_NAME_SIZE 256
#define VK_MAX_DESCRIPTION_SIZE 256
#define VK_UUID_SIZE 16
#define VK_SUCCESS 0

typedef void* VkPhysicalDevice;
typedef int VkResult;
typedef int VkStructureType;
typedef int VkDriverId;

#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2      ((VkStructureType)1000059001)
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES ((VkStructureType)1000194000)
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES ((VkStructureType)1000070001)
#define VK_DRIVER_ID_ARM_PROPRIETARY ((VkDriverId)1000000003)

typedef struct VkBaseOutStructure {
    VkStructureType sType;
    struct VkBaseOutStructure* pNext;
} VkBaseOutStructure;

typedef struct VkPhysicalDeviceProperties {
    uint32_t apiVersion;
    uint32_t driverVersion;
    uint32_t vendorID;
    uint32_t deviceID;
    uint32_t deviceType;
    char     deviceName[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
    uint8_t  pipelineCacheUUID[VK_UUID_SIZE];
    uint8_t  _reserved[512]; // 后续字段占位 不访问
} VkPhysicalDeviceProperties;

typedef struct VkPhysicalDeviceProperties2 {
    VkStructureType sType;
    void* pNext;
    VkPhysicalDeviceProperties properties;
} VkPhysicalDeviceProperties2;

typedef struct VkPhysicalDeviceDriverProperties {
    VkStructureType sType;
    void* pNext;
    VkDriverId driverID;
    char driverName[VK_MAX_DESCRIPTION_SIZE];
    char driverInfo[VK_MAX_DESCRIPTION_SIZE];
    uint8_t _reserved[32]; // 后续字段占位
} VkPhysicalDeviceDriverProperties;

typedef struct VkPhysicalDeviceVulkan11Properties {
    VkStructureType sType;
    void* pNext;
    uint8_t deviceUUID[VK_UUID_SIZE];
    uint8_t _reserved[256]; // 后续字段占位
} VkPhysicalDeviceVulkan11Properties;

// 函数指针类型定义
typedef VkResult (*PFN_vkGetPhysicalDeviceProperties2)(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2* pProperties2);
static PFN_vkGetPhysicalDeviceProperties2 orig_vkGetPhysicalDeviceProperties2 = nullptr;

// ========== 伪造Vulkan属性回调 ==========
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

// ========== Zygisk模块主类 ==========
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

        const char* pkg_name = env->GetStringUTFChars(args->package_name, nullptr);
        bool target_app = false;

        if (pkg_name != nullptr) {
            // 目标应用：三角洲行动主包、硬件检测工具
            if (strcmp(pkg_name, "com.tencent.tmgp.dfm") == 0) target_app = true;
            if (strcmp(pkg_name, "com.liuzh.deviceinfo") == 0) target_app = true;

            // 匹配游戏分身进程 :game
            const char* proc_name = env->GetStringUTFChars(args->nice_name, nullptr);
            if (proc_name != nullptr) {
                if (strstr(proc_name, "com.tencent.tmgp.dfm:game")) {
                    target_app = true;
                }
                env->ReleaseStringUTFChars(args->nice_name, proc_name);
            }
        }

        // 仅目标进程挂载Vulkan GPU伪装Hook
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

        // 安全释放字符串
        if (pkg_name != nullptr) {
            env->ReleaseStringUTFChars(args->package_name, pkg_name);
        }
    }

private:
    Api* api;
    JNIEnv* env;
};
// 单行无拆分 杜绝预处理器语法断裂
REGISTER_ZYGISK_MODULE(KirinSpoofModule);
