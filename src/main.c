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

#include "allocator.h"
#include "renderer.h"

const uint32_t WIN_WIDTH = 800;
const uint32_t WIN_HEIGHT = 600;

const bool VALIDATION_LAYERS_ENABLED = true;
const char *VALIDATION_LAYERS[] = {"VK_LAYER_KHRONOS_validation"};

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
               VkDebugUtilsMessageTypeFlagsEXT messageType,
               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {

    printf("Validation: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
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

Array get_instance_required_extensions() {
    uint32_t extension_count = 0;
    char **extensions = (char **)glfwGetRequiredInstanceExtensions(&extension_count);

    if (VALIDATION_LAYERS_ENABLED) {
        char **new_extensions = malloc((extension_count + 1) * sizeof(char *));
        memcpy(new_extensions, extensions, extension_count * sizeof(char *));
        new_extensions[extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        extensions = new_extensions;
        extension_count++;
    }

    return (Array){
        .data = extensions,
        .count = extension_count,
        .capacity = extension_count,
        .size = sizeof(char *),
    };
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

    Array extensions = get_instance_required_extensions();
    create_info.enabledExtensionCount = extensions.count;
    create_info.ppEnabledExtensionNames = (const char **)extensions.data;

    for (int i = 0; i < extensions.count; i++) {
        printf("Extension name: %s\n", *(char **)array_get(&extensions, i));
    }

    VkDebugUtilsMessengerCreateInfoEXT debug_info;

    if (VALIDATION_LAYERS_ENABLED) {
        create_info.ppEnabledLayerNames = (void *)&VALIDATION_LAYERS;
        create_info.enabledLayerCount = sizeof(VALIDATION_LAYERS) / sizeof(char *);

        init_debug_create_info(&debug_info);
        create_info.pNext = &debug_info;
    } else {
        create_info.enabledLayerCount = 0;
    }

    return vkCreateInstance(&create_info, NULL, out);
}

VkResult create_debug_utils_messenger_ext(VkInstance instance,
                                          const VkDebugUtilsMessengerCreateInfoEXT *p_create_info,
                                          const VkAllocationCallbacks *p_allocator,
                                          VkDebugUtilsMessengerEXT *p_debug_messenger) {

    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
                                                                  "vkCreateDebugUtilsMessengerEXT");
    if (func) {
        return func(instance, p_create_info, p_allocator, p_debug_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroy_debug_utils_messenger_ext(VkInstance instance,
                                       VkDebugUtilsMessengerEXT debug_messenger,
                                       const VkAllocationCallbacks *p_allocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func) {
        func(instance, debug_messenger, p_allocator);
    }
}

VkResult create_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT *out) {
    VkDebugUtilsMessengerCreateInfoEXT create_info;
    init_debug_create_info(&create_info);
    return create_debug_utils_messenger_ext(instance, &create_info, NULL, out);
}

bool init_libraries() {
    if (volkInitialize() != VK_SUCCESS) {
        return false;
    }

    if (glfwInit() != GLFW_TRUE) {
        const char *description;
        int code = glfwGetError(&description);
        printf("GLFW initialization failed!\n");
        printf("Error %d: %s\n", code, description ? description : "Unknown error");
        return false;
    }

    return true;
}

int main() {
    assert(init_libraries());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    VkInstance instance;
    assert(create_vk_instance(&instance) == VK_SUCCESS);
    volkLoadInstance(instance);

    VkDebugUtilsMessengerEXT debug_messenger;
    if (VALIDATION_LAYERS_ENABLED) {
        assert(create_debug_messenger(instance, &debug_messenger) == VK_SUCCESS);
    }

    GLFWwindow *window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Vulkan window", NULL, NULL);
    assert(window);

    VkSurfaceKHR surface;
    assert(glfwCreateWindowSurface(instance, window, NULL, &surface) == VK_SUCCESS);

    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    RendererState renderer;
    assert(
        renderer_create(std_allocator, instance, surface, window_width, window_height, &renderer));

	printf("Renderer created\n");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
		renderer_draw(&renderer);
    }

    if (VALIDATION_LAYERS_ENABLED) {
        destroy_debug_utils_messenger_ext(instance, debug_messenger, NULL);
    }

    renderer_destroy(std_allocator, &renderer);
    vkDestroyInstance(instance, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
}
