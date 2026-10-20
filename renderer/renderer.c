
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <volk.h>

#include "allocator.h"
#include "array.h"
#include "renderer.h"

#include "spirv/shader.frag.h"
#include "spirv/shader.vert.h"

const char *REQUIRED_EXTENSIONS[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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

VkExtent2D choose_swap_surface_extent(VkSurfaceCapabilitiesKHR capabilities, int window_width,
                                      int window_height) {
    uint32_t width = window_width;
    uint32_t height = window_height;

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
                          int window_width, int window_height, SwapchainDetails *out) {
    SwapchainSupportDetails swapchain_support =
        query_swapchain_support(std_allocator, physical_device, surface);
    VkSurfaceFormatKHR surface_format =
        choose_swap_surface_format(&swapchain_support.surface_formats);
    VkPresentModeKHR present_mode = choose_swap_present_mode(&swapchain_support.present_modes);
    VkExtent2D surface_extent =
        choose_swap_surface_extent(swapchain_support.capabilities, window_width, window_height);

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

    VkSwapchainKHR swapchain;
    VkResult result = vkCreateSwapchainKHR(device, &create_info, NULL, &swapchain);
    if (result == VK_SUCCESS) {
        *out = (SwapchainDetails){
            .support = swapchain_support,
            .swapchain = swapchain,
            .present_mode = present_mode,
            .surface_extent = surface_extent,
            .surface_format = surface_format,
        };
    }

    return result;
}

Array retrieve_swapchain_images(Allocator allocator, VkDevice device, VkSwapchainKHR swapchain) {
    Array images;
    uint32_t image_count;
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, NULL);
    array_allocate(image_count, sizeof(VkImage), allocator, &images);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, images.data);
    images.count = image_count;
    return images;
}

VkResult create_image_views(Allocator allocator, VkDevice device, VkFormat image_format,
                            Array swapchain_images, Array *out_views) {
    Array image_views = {0};
    array_allocate(swapchain_images.count, sizeof(VkImageView), allocator, &image_views);

    for (int i = 0; i < swapchain_images.count; i++) {
        VkImageViewCreateInfo create_info = {0};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = *(VkImage *)array_get_item(&swapchain_images, i);
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = image_format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        VkResult result =
            vkCreateImageView(device, &create_info, NULL, array_append_item(&image_views));
        if (result != VK_SUCCESS) {
            array_free(&image_views, allocator);
            return result;
        }
    }

    *out_views = image_views;

    return VK_SUCCESS;
}

VkResult create_shader_module(VkDevice device, uint32_t code_size, char *code,
                              VkShaderModule *out) {
    VkShaderModuleCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code_size;
    create_info.pCode = (const uint32_t *)code;
    return vkCreateShaderModule(device, &create_info, NULL, out);
}

// TODO: destroy objects in case of failure
bool renderer_create(Allocator allocator, VkInstance instance, VkSurfaceKHR surface,
                     int window_width, int window_height, RendererState *out) {
    PhysicalDeviceDetails physical_device_details = pick_physical_device(instance, surface);
    if (physical_device_details.physical_device == VK_NULL_HANDLE)
        return false;

    VkPhysicalDevice physical_device = physical_device_details.physical_device;
    QueueFamilyIndices queue_families = physical_device_details.queue_families;

    VkDevice device;
    if (create_logical_device(physical_device, queue_families, &device) != VK_SUCCESS)
        return false;

    VkQueue device_queue;
    vkGetDeviceQueue(device, queue_families.family, 0, &device_queue);

    SwapchainDetails swapchain_details;
    if (create_swapchain(physical_device, device, surface, window_width, window_height,
                         &swapchain_details) != VK_SUCCESS) {
        vkDestroyDevice(device, NULL);
        return false;
    }

    VkSwapchainKHR swapchain = swapchain_details.swapchain;

    Array swapchain_images = retrieve_swapchain_images(std_allocator, device, swapchain);
    Array image_views;
    if (create_image_views(std_allocator, device, swapchain_details.surface_format.format,
                           swapchain_images, &image_views) != VK_SUCCESS)
        return false;

	VkShaderModule fragment_shader;
	if (create_shader_module(device, SHADER_CONTENTS_shader_frag_len, (char *)SHADER_CONTENTS_shader_frag, &fragment_shader) != VK_SUCCESS)
		return false;


	VkShaderModule vertex_shader;
	if (create_shader_module(device, SHADER_CONTENTS_shader_vert_len, (char *)SHADER_CONTENTS_shader_vert, &vertex_shader) != VK_SUCCESS)
		return false;

    RendererState renderer = {
        .swapchain_details = swapchain_details,
        .swapchain_images = swapchain_images,
        .swapchain_views = image_views,
        .device = device,
        .device_queue = device_queue,
        .physical_device = physical_device_details,
        .surface = surface,

		.frag = fragment_shader,
		.vert = vertex_shader,
    };

    return true;
}

void renderer_destroy(Allocator allocator, RendererState *renderer) {
    for (int i = 0; i < renderer->swapchain_views.count; i++) {
        vkDestroyImageView(renderer->device,
                           *(VkImageView *)array_get_item(&renderer->swapchain_views, i), NULL);
    }

    vkDestroySwapchainKHR(renderer->device, renderer->swapchain_details.swapchain, NULL);
    vkDestroyDevice(renderer->device, NULL);

	vkDestroyShaderModule(renderer->device, renderer->frag, NULL);
	vkDestroyShaderModule(renderer->device, renderer->vert, NULL);

    free_swapchain_support_details(allocator, &renderer->swapchain_details.support);
    array_free(&renderer->swapchain_images, allocator);
    array_free(&renderer->swapchain_views, allocator);
}
