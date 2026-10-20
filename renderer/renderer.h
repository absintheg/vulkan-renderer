#ifndef CLIENT_H
#define CLIENT_H

#include "array.h"
#include <stdbool.h>
#include <vulkan/vulkan.h>

typedef struct QueueFamilyIndices {
    bool available;
    uint32_t family;
} QueueFamilyIndices;

typedef struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    Array present_modes;
    Array surface_formats;
} SwapchainSupportDetails;

typedef struct SwapchainDetails {
    SwapchainSupportDetails support;
    VkExtent2D surface_extent;
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;
    VkSwapchainKHR swapchain;
} SwapchainDetails;

typedef struct PhysicalDeviceDetails {
    QueueFamilyIndices queue_families;
    VkPhysicalDevice physical_device;
} PhysicalDeviceDetails;

typedef struct SyncObjects {
    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;
} SyncObjects;

typedef struct RendererState {
    VkDevice device;
    VkQueue device_queue;
    VkSurfaceKHR surface;
    VkInstance instance;
    SwapchainDetails swapchain_details;
    PhysicalDeviceDetails physical_device;

    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;

    Array swapchain_images;
    Array swapchain_views;
    Array framebuffers;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;

    SyncObjects sync_objects;

    VkShaderModule frag;
    VkShaderModule vert;
} RendererState;

bool renderer_create(Allocator allocator, VkInstance instance, VkSurfaceKHR surface,
                     int window_width, int window_height, RendererState *out);

bool renderer_draw(RendererState *renderer);

void renderer_destroy(Allocator allocator, RendererState *renderer);

#endif
