#define GLFW_INCLUDE_VULKAN
#define VK_NO_PROTOTYPES

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLFW/glfw3.h>
#include <volk.h>

const uint32_t WIN_WIDTH = 800;
const uint32_t WIN_HEIGHT = 600;

const bool validation_layers_enabled = true;
const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
               VkDebugUtilsMessageTypeFlagsEXT messageType,
               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
               void *pUserData) {

    printf("validation: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

typedef struct {
    char **extensions;
    uint32_t count;
} ExtensionList;

bool check_validation_support() {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);

    VkLayerProperties *available_layers =
        malloc(layer_count * sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

    for (int i = 0; i < layer_count; i++) {
        printf("\t Validation layer name: %s\n", available_layers[i].layerName);
        printf("\t            layer desc: %s\n",
               available_layers[i].description);
        printf("\n");
    }

    for (int i = 0; i < sizeof(validation_layers) / sizeof(char *); i++) {
        bool layer_found = false;

        for (int j = 0; j < layer_count; j++) {
            if (strcmp(available_layers[j].layerName, validation_layers[i]) ==
                0) {
                layer_found = true;
                break;
            }
        }
        if (!layer_found) {
            return false;
        }
    }

    return true;
}

ExtensionList get_required_extensions() {
    uint32_t extension_count = 0;
    const char **extensions;
    extensions = glfwGetRequiredInstanceExtensions(&extension_count);

    if (validation_layers_enabled) {
        const char **new_extensions =
            malloc((extension_count + 1) * sizeof(char *));
        memcpy(new_extensions, extensions, extension_count * sizeof(char *));
        new_extensions[extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        extensions = new_extensions;
        extension_count++;
    }

    return (ExtensionList){
        .extensions = (char **)extensions,
        .count = extension_count,
    };
}

VkResult create_debug_utils_messenger_ext(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *p_create_info,
    const VkAllocationCallbacks *p_allocator,
    VkDebugUtilsMessengerEXT *p_debug_messenger) {

    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
    if (func) {
        return func(instance, p_create_info, p_allocator, p_debug_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroy_debug_utils_messenger_ext(
    VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
    const VkAllocationCallbacks *p_allocator) {

    PFN_vkDestroyDebugUtilsMessengerEXT func =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func) {
        func(instance, debug_messenger, p_allocator);
    }
}

void init_debug_create_info(VkDebugUtilsMessengerCreateInfoEXT *info) {
    *info = (VkDebugUtilsMessengerCreateInfoEXT){0};
    info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info->pfnUserCallback = debug_callback;
    info->pUserData = NULL; // Optional
}

VkResult create_debug_messenger(VkInstance instance,
                                VkDebugUtilsMessengerEXT *out) {

    VkDebugUtilsMessengerCreateInfoEXT create_info;
    init_debug_create_info(&create_info);

    return create_debug_utils_messenger_ext(instance, &create_info, NULL, out);
}

VkResult create_vk_instance(VkInstance *out) {
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    ExtensionList extensions = get_required_extensions();
    create_info.enabledExtensionCount = extensions.count;
    create_info.ppEnabledExtensionNames = (const char **)extensions.extensions;

    VkDebugUtilsMessengerCreateInfoEXT debug_info;

    if (validation_layers_enabled) {
        create_info.ppEnabledLayerNames = (void *)&validation_layers;
        create_info.enabledLayerCount =
            sizeof(validation_layers) / sizeof(char *);

        init_debug_create_info(&debug_info);
        create_info.pNext = &debug_info;
    } else {
        create_info.enabledLayerCount = 0;
    }

    return vkCreateInstance(&create_info, NULL, out);
}

bool rate_device_suitablity(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceProperties(device, &device_properties);
    vkGetPhysicalDeviceFeatures(device, &device_features);

    int score = 0;

    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    score += device_properties.limits.maxImageDimension2D;

    return score;
}

VkPhysicalDevice pick_physical_device(VkInstance instance) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    VkPhysicalDevice devices[device_count];
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    int max_score = 0;
    int max_score_device = -1;

    for (int i = 0; i < device_count; i++) {
        int score = rate_device_suitablity(devices[i]);
        if (score > max_score) {
            max_score = score;
            max_score_device = i;
        }
    }

    if (max_score == 0) {
        return VK_NULL_HANDLE;
    }

    return devices[device_count];
}

int start_client() {
    assert(volkInitialize() == VK_SUCCESS);

    if (validation_layers_enabled && !check_validation_support()) {
        printf("Validation layers not supported\n");
        return -1;
    }

    if (glfwInit() != GLFW_TRUE) {
        const char *description;
        int code = glfwGetError(&description);
        printf("GLFW initialization failed!\n");
        printf("Error %d: %s\n", code,
               description ? description : "Unknown error");
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow *window =
        glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Vulkan window", NULL, NULL);

    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
    printf("Extension count: %d\n", extension_count);

    VkExtensionProperties *extension_list =
        malloc(extension_count * sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count,
                                           extension_list);

    for (int i = 0; i < extension_count; i++) {
        printf("\tExtension name: %s\n", extension_list[i].extensionName);
    }

    VkInstance instance;
    assert(create_vk_instance(&instance) == VK_SUCCESS);

    VkDebugUtilsMessengerEXT debug_messenger;
    if (validation_layers_enabled) {
        assert(create_debug_messenger(instance, &debug_messenger) ==
               VK_SUCCESS);
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    if (validation_layers_enabled) {
        destroy_debug_utils_messenger_ext(instance, debug_messenger, NULL);
    }

    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
