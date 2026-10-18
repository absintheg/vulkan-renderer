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
#include "array.h"

const uint32_t WIN_WIDTH = 800;
const uint32_t WIN_HEIGHT = 600;

const bool VALIDATION_LAYERS_ENABLED = true;
const char *VALIDATION_LAYERS[] = {"VK_LAYER_KHRONOS_validation"};
const char *REQUIRED_EXTENSIONS[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

typedef struct ExtensionList {
    char **extensions;
    uint32_t count;
} ExtensionList;

typedef struct QueueFamilyIndices {
    bool available;
    uint32_t family;
} QueueFamilyIndices;

typedef struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    Array present_modes;
    Array surface_formats;
} SwapchainSupportDetails;

typedef struct PhysicalDeviceDetails {
    QueueFamilyIndices queue_families;
    VkPhysicalDevice physical_device;
} PhysicalDeviceDetails;

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
               VkDebugUtilsMessageTypeFlagsEXT messageType,
               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {

    printf("Validation: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

bool check_validation_support() {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);

    VkLayerProperties available_layers[layer_count];
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

    for (int i = 0; i < layer_count; i++) {
        printf("\t Validation layer name: %s\n", available_layers[i].layerName);
        printf("\t            layer desc: %s\n", available_layers[i].description);
        printf("\n");
    }

    for (int i = 0; i < sizeof(VALIDATION_LAYERS) / sizeof(char *); i++) {
        bool layer_found = false;

        for (int j = 0; j < layer_count; j++) {
            if (strcmp(available_layers[j].layerName, VALIDATION_LAYERS[i]) == 0) {
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

ExtensionList get_instance_required_extensions() {
    uint32_t extension_count = 0;
    char **extensions = (char **)glfwGetRequiredInstanceExtensions(&extension_count);

    if (VALIDATION_LAYERS_ENABLED) {
        char **new_extensions = malloc((extension_count + 1) * sizeof(char *));
        memcpy(new_extensions, extensions, extension_count * sizeof(char *));
        new_extensions[extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        extensions = new_extensions;
        extension_count++;
    }

    return (ExtensionList){
        .extensions = extensions,
        .count = extension_count,
    };
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

VkResult create_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT *out) {

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

    ExtensionList extensions = get_instance_required_extensions();
    create_info.enabledExtensionCount = extensions.count;
    create_info.ppEnabledExtensionNames = (const char **)extensions.extensions;

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

QueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    QueueFamilyIndices indices;

    for (int i = 0; i < queue_family_count; i++) {
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
        if (present_support && queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.available = true;
            indices.family = i;
            break;
        }
    }

    return indices;
}

bool check_device_extensions(VkPhysicalDevice device, VkSurfaceKHR surface) {
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);

    VkExtensionProperties extensions[extension_count];
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, extensions);
    for (int i = 0; i < sizeof(REQUIRED_EXTENSIONS) / sizeof(char *); i++) {
        bool found = false;
        for (int j = 0; j < extension_count; j++) {
            if (strcmp(extensions[j].extensionName, REQUIRED_EXTENSIONS[i]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    uint32_t surface_format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surface_format_count, NULL);
    if (surface_format_count == 0) {
        return false;
    }

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, NULL);
    if (present_mode_count == 0) {
        return false;
    }

    return true;
}

SwapchainSupportDetails query_swapchain_support(Allocator allocator, VkPhysicalDevice device,
                                                VkSurfaceKHR surface) {
    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    uint32_t surface_format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surface_format_count, NULL);

    if (surface_format_count != 0) {
        array_allocate(surface_format_count, sizeof(VkSurfaceFormatKHR), allocator,
                       &details.surface_formats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surface_format_count,
                                             details.surface_formats.data);
        details.surface_formats.count = surface_format_count;
    }

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, NULL);

    if (present_mode_count != 0) {
        array_allocate(surface_format_count, sizeof(VkPresentModeKHR), allocator,
                       &details.present_modes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count,
                                                  details.present_modes.data);
        details.present_modes.count = surface_format_count;
    }

    return details;
}

VkSurfaceFormatKHR choose_swap_surface_format(Array *available_formats) {
    for (int i = 0; i < available_formats->count; i++) {
        VkSurfaceFormatKHR format = *(VkSurfaceFormatKHR *)array_get_item(available_formats, i);

        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }
    return *(VkSurfaceFormatKHR *)array_get_first(available_formats);
}

VkPresentModeKHR choose_swap_present_mode(Array *available_modes) {
    for (int i = 0; i < available_modes->count; i++) {
        VkPresentModeKHR mode = *(VkPresentModeKHR *)array_get_item(available_modes, i);

        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_swap_surface_extent(VkSurfaceCapabilitiesKHR capabilities, GLFWwindow *window) {
    int gwidth, gheight;
    glfwGetFramebufferSize(window, &gwidth, &gheight);

    uint32_t width = gwidth;
    uint32_t height = gheight;

    if (width < capabilities.minImageExtent.width)
        width = capabilities.minImageExtent.width;
    else if (width > capabilities.maxImageExtent.width)
        width = capabilities.maxImageExtent.width;

    if (height < capabilities.minImageExtent.height)
        height = capabilities.minImageExtent.height;
    else if (height > capabilities.maxImageExtent.height)
        height = capabilities.maxImageExtent.height;

    VkExtent2D extent = {width, height};
    return extent;
}

void free_swapchain_support_details(Allocator allocator, SwapchainSupportDetails *details) {
    array_free(&details->surface_formats, allocator);
    array_free(&details->present_modes, allocator);
}

bool rate_device_suitablity(VkPhysicalDevice device, QueueFamilyIndices indices,
                            VkSurfaceKHR surface) {
    if (!indices.available || !check_device_extensions(device, surface)) {
        return 0;
    }

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);

    int score = 0;
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }
    score += device_properties.limits.maxImageDimension2D;

    return score;
}

PhysicalDeviceDetails pick_physical_device(VkInstance instance, VkSurfaceKHR surface) {

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    VkPhysicalDevice devices[device_count];
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    int max_score = 0;
    PhysicalDeviceDetails max_score_result;

    for (int i = 0; i < device_count; i++) {
        QueueFamilyIndices queue_families = find_queue_families(devices[i], surface);

        int score = rate_device_suitablity(devices[i], queue_families, surface);
        if (score > max_score) {
            max_score = score;
            max_score_result = (PhysicalDeviceDetails){
                .physical_device = devices[i],
                .queue_families = queue_families,
            };
        }
    }

    return max_score_result;
}

VkDeviceQueueCreateInfo create_queue_creation_info(uint32_t family) {
    VkDeviceQueueCreateInfo queue_create_info = {0};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = family;
    queue_create_info.queueCount = 1;
    static float queue_priority = 1.0;
    queue_create_info.pQueuePriorities = &queue_priority;
    return queue_create_info;
}

VkResult create_logical_device(VkPhysicalDevice physical_device, QueueFamilyIndices indices,
                               VkDevice *out) {

    VkPhysicalDeviceFeatures requested_features = {0};
    VkDeviceQueueCreateInfo queue_create_info = create_queue_creation_info(indices.family);

    VkDeviceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pEnabledFeatures = &requested_features;
    create_info.pQueueCreateInfos = &queue_create_info;
    create_info.queueCreateInfoCount = 1;

    create_info.enabledExtensionCount = sizeof(REQUIRED_EXTENSIONS) / sizeof(char *);
    create_info.ppEnabledExtensionNames = REQUIRED_EXTENSIONS;

    // if (validation_layers_enabled) {
    //     create_info.enabledLayerCount = sizeof(validation_layers) / sizeof(char *);
    //     create_info.ppEnabledLayerNames = validation_layers;
    // } else {
    //     create_info.enabledLayerCount = 0;
    // }

    return vkCreateDevice(physical_device, &create_info, NULL, out);
}

VkResult create_swapchain(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface,
                          GLFWwindow *window, VkSwapchainKHR *out) {
    SwapchainSupportDetails swapchain_support =
        query_swapchain_support(std_allocator, physical_device, surface);
    VkSurfaceFormatKHR surface_format =
        choose_swap_surface_format(&swapchain_support.surface_formats);
    VkPresentModeKHR present_mode = choose_swap_present_mode(&swapchain_support.present_modes);
    VkExtent2D surface_extent = choose_swap_surface_extent(swapchain_support.capabilities, window);

    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0 &&
        swapchain_support.capabilities.maxImageCount < image_count)
        image_count = swapchain_support.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = image_count;
    create_info.imageExtent = surface_extent;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    create_info.preTransform = swapchain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;

	create_info.oldSwapchain = VK_NULL_HANDLE;

    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;  // Optional
    create_info.pQueueFamilyIndices = NULL; // Optional

    VkResult result = vkCreateSwapchainKHR(device, &create_info, NULL, out);
	free_swapchain_support_details(std_allocator, &swapchain_support);
	return result;
}

int start_client() {
    assert(volkInitialize() == VK_SUCCESS);

    if (VALIDATION_LAYERS_ENABLED && !check_validation_support()) {
        printf("Validation layers not supported\n");
        return -1;
    }

    if (glfwInit() != GLFW_TRUE) {
        const char *description;
        int code = glfwGetError(&description);
        printf("GLFW initialization failed!\n");
        printf("Error %d: %s\n", code, description ? description : "Unknown error");
        return -1;
    }

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

    VkSurfaceKHR surface;
    assert(glfwCreateWindowSurface(instance, window, NULL, &surface) == VK_SUCCESS);

    PhysicalDeviceDetails physical_device_details = pick_physical_device(instance, surface);
    assert(physical_device_details.physical_device != VK_NULL_HANDLE);

    VkPhysicalDevice physical_device = physical_device_details.physical_device;
    QueueFamilyIndices queue_families = physical_device_details.queue_families;

    VkDevice device;
    assert(create_logical_device(physical_device, queue_families, &device) == VK_SUCCESS);

    VkQueue device_queue;
    vkGetDeviceQueue(device, queue_families.family, 0, &device_queue);

    VkSwapchainKHR swapchain;
    assert(create_swapchain(physical_device, device, surface, window, &swapchain) == VK_SUCCESS);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    if (VALIDATION_LAYERS_ENABLED) {
        destroy_debug_utils_messenger_ext(instance, debug_messenger, NULL);
    }

    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
