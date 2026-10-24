
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
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
        array_allocate(&details.surface_formats, surface_format_count, sizeof(VkSurfaceFormatKHR),
                       allocator);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surface_format_count,
                                             details.surface_formats.data);
        details.surface_formats.count = surface_format_count;
    } else {
        printf("Zero format\n");
    }

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, NULL);

    if (present_mode_count != 0) {
        array_allocate(&details.present_modes, surface_format_count, sizeof(VkPresentModeKHR),
                       allocator);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count,
                                                  details.present_modes.data);
        details.present_modes.count = surface_format_count;
    }

    return details;
}

VkSurfaceFormatKHR choose_swap_surface_format(Array *available_formats) {
    for (int i = 0; i < available_formats->count; i++) {
        VkSurfaceFormatKHR format = *(VkSurfaceFormatKHR *)array_get(available_formats, i);

        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }
    return *(VkSurfaceFormatKHR *)array_first(available_formats);
}

VkPresentModeKHR choose_swap_present_mode(Array *available_modes) {
    for (int i = 0; i < available_modes->count; i++) {
        VkPresentModeKHR mode = *(VkPresentModeKHR *)array_get(available_modes, i);

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
    array_allocate(&images, image_count, sizeof(VkImage), allocator);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, images.data);
    images.count = image_count;
    return images;
}

void destroy_image_views(Allocator allocator, VkDevice device, Array *image_views) {
    // Iterate in reverse because some allocator implementations
    // work better this way
    for (int i = image_views->count - 1; i >= 0; i--) {
        vkDestroyImageView(device, *(VkImageView *)array_get(image_views, i), NULL);
    }
    array_free(image_views, allocator);
}

VkResult create_image_views(Allocator allocator, VkDevice device, VkFormat image_format,
                            Array swapchain_images, Array *out_views) {
    Array image_views = {0};
    array_allocate(&image_views, swapchain_images.count, sizeof(VkImageView), allocator);

    for (int i = 0; i < swapchain_images.count; i++) {
        VkImageViewCreateInfo create_info = {0};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = *(VkImage *)array_get(&swapchain_images, i);
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

        VkResult result = vkCreateImageView(device, &create_info, NULL, array_append(&image_views));
        if (result != VK_SUCCESS) {
            destroy_image_views(allocator, device, &image_views);
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

VkResult create_graphics_pipeline_layout(VkDevice device, VkPipelineLayout *out) {
    VkPipelineLayout pipeline_layout;

    VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    return vkCreatePipelineLayout(device, &pipeline_layout_info, NULL, out);
}

VkResult create_render_pass(VkDevice device, SwapchainDetails swapchain_details,
                            VkRenderPass *out) {
    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = swapchain_details.surface_format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {0};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;

    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    return vkCreateRenderPass(device, &render_pass_info, NULL, out);
}

VkRect2D create_scissor(VkExtent2D extent) {
    VkRect2D scissor = {0};
    scissor.offset = (VkOffset2D){0, 0};
    scissor.extent = extent;
    return scissor;
}

VkViewport create_viewport(VkExtent2D extent) {
    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

VkResult create_graphics_pipeline(VkDevice device, VkShaderModule frag, VkShaderModule vert,
                                  VkExtent2D extent, VkPipelineLayout layout, VkRenderPass pass,
                                  VkPipeline *out) {
    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {0};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert;
    vert_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {0};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag;
    frag_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info,
                                                       frag_shader_stage_info};

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {0};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkDynamicState dynamic_state_list[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state = {0};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = sizeof(dynamic_state_list) / sizeof(VkDynamicState);
    dynamic_state.pDynamicStates = dynamic_state_list;

    VkPipelineViewportStateCreateInfo viewport_state = {0};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f;          // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE;      // Optional

    VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending = {0};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY; // Optional
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;

    VkGraphicsPipelineCreateInfo pipeline_info = {0};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;

    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;

    pipeline_info.layout = layout;

    pipeline_info.renderPass = pass;
    pipeline_info.subpass = 0;

    return vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, out);
}

void destroy_framebuffers(Allocator allocator, VkDevice device, Array *framebuffers) {
    for (int i = framebuffers->count - 1; i >= 0; i--) {
        vkDestroyFramebuffer(device, array_get(framebuffers, i), NULL);
    }
    array_free(framebuffers, allocator);
}

VkResult create_framebuffers(Allocator allocator, VkDevice device, Array image_views,
                             VkRenderPass render_pass, VkExtent2D surface_extent, Array *out) {
    Array framebuffers = {0};
    array_allocate(&framebuffers, image_views.count, sizeof(VkFramebuffer), allocator);

    for (int i = 0; i < image_views.count; i++) {
        VkFramebufferCreateInfo framebuffer_info = {0};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = (VkImageView *)array_get(&image_views, i);
        framebuffer_info.width = surface_extent.width;
        framebuffer_info.height = surface_extent.height;
        framebuffer_info.layers = 1;

        VkResult result =
            vkCreateFramebuffer(device, &framebuffer_info, NULL, array_append(&framebuffers));
        if (result != VK_SUCCESS) {
            destroy_framebuffers(allocator, device, &framebuffers);
            return result;
        }
    }

    *out = framebuffers;

    return VK_SUCCESS;
}

VkResult create_command_pool(VkDevice device, QueueFamilyIndices queue_families,
                             VkCommandPool *out) {
    VkCommandPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_families.family;
    return vkCreateCommandPool(device, &pool_info, NULL, out);
}

VkResult create_command_buffer(VkDevice device, VkCommandPool command_pool, VkCommandBuffer *out) {
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.commandBufferCount = 1;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    return vkAllocateCommandBuffers(device, &alloc_info, out);
}

bool record_renderer_command_buffer(RendererState *renderer, int image_index) {
    VkCommandBuffer command_buffer = renderer->command_buffer;
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
        goto fail;

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline);

    VkRenderPassBeginInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = renderer->render_pass;
    render_pass_info.framebuffer =
        *(VkFramebuffer *)array_get(&renderer->framebuffers, image_index);

    render_pass_info.renderArea.offset = (VkOffset2D){0, 0};
    render_pass_info.renderArea.extent = renderer->swapchain_details.surface_extent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clearColor;
    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = create_viewport(renderer->swapchain_details.surface_extent);
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = create_scissor(renderer->swapchain_details.surface_extent);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdDraw(command_buffer, 6, 1, 0, 0);
    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
        goto fail;

fail:
    return false;
}

VkResult create_sync_objects(VkDevice device, SyncObjects *out) {
    SyncObjects objects;

    VkSemaphoreCreateInfo semaphore_info = {0};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult result;

    result = vkCreateSemaphore(device, &semaphore_info, NULL, &objects.image_available_semaphore);
    if (result != VK_SUCCESS)
        goto end;

    result = vkCreateSemaphore(device, &semaphore_info, NULL, &objects.render_finished_semaphore);
    if (result != VK_SUCCESS)
        goto cleanup_1;

    result = vkCreateFence(device, &fence_info, NULL, &objects.in_flight_fence);
    if (result != VK_SUCCESS)
        goto cleanup_2;

    *out = objects;
    return VK_SUCCESS;

cleanup_2:
    vkDestroySemaphore(device, objects.render_finished_semaphore, NULL);
cleanup_1:
    vkDestroySemaphore(device, objects.image_available_semaphore, NULL);
end:
    return result;
}

void destroy_sync_objects(VkDevice device, SyncObjects objects) {
    vkDestroyFence(device, objects.in_flight_fence, NULL);
    vkDestroySemaphore(device, objects.render_finished_semaphore, NULL);
    vkDestroySemaphore(device, objects.image_available_semaphore, NULL);
}

bool renderer_draw(RendererState *renderer) {
    VkDevice device = renderer->device;
    VkSwapchainKHR swapchain = renderer->swapchain_details.swapchain;
    VkCommandBuffer command_buffer = renderer->command_buffer;
    SyncObjects sync_objects = renderer->sync_objects;

    vkWaitForFences(device, 1, &sync_objects.in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &sync_objects.in_flight_fence);

    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    uint32_t image_index;
    VkResult acquire_result =
        vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, sync_objects.image_available_semaphore,
                              VK_NULL_HANDLE, &image_index);
    if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR)
        return false;

    vkResetCommandBuffer(command_buffer, 0);

    record_renderer_command_buffer(renderer, image_index);

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {sync_objects.image_available_semaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = waitStages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    VkSemaphore signal_semaphores[] = {sync_objects.render_finished_semaphore};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(renderer->device_queue, 1, &submit_info, sync_objects.in_flight_fence) !=
        VK_SUCCESS) {
        return false;
    }

    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapchains[] = {swapchain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;

    vkQueuePresentKHR(renderer->device_queue, &present_info);

    return true;
}

bool renderer_create(Allocator allocator, VkInstance instance, VkSurfaceKHR surface,
                     int window_width, int window_height, RendererState *out) {
    PhysicalDeviceDetails physical_device_details = pick_physical_device(instance, surface);
    if (physical_device_details.physical_device == VK_NULL_HANDLE)
        goto fail;

    VkPhysicalDevice physical_device = physical_device_details.physical_device;
    QueueFamilyIndices queue_families = physical_device_details.queue_families;

    VkDevice device;
    if (create_logical_device(physical_device, queue_families, &device) != VK_SUCCESS)
        goto fail;

    VkQueue device_queue;
    vkGetDeviceQueue(device, queue_families.family, 0, &device_queue);

    SwapchainDetails swapchain_details;
    if (create_swapchain(physical_device, device, surface, window_width, window_height,
                         &swapchain_details) != VK_SUCCESS)
        goto cleanup_device;

    VkSwapchainKHR swapchain = swapchain_details.swapchain;

    Array swapchain_images = retrieve_swapchain_images(std_allocator, device, swapchain);
    Array image_views;
    if (create_image_views(std_allocator, device, swapchain_details.surface_format.format,
                           swapchain_images, &image_views) != VK_SUCCESS)
        goto cleanup_swapchain;

    VkShaderModule fragment_shader;
    if (create_shader_module(device, SHADER_CONTENTS_shader_frag_len,
                             (char *)SHADER_CONTENTS_shader_frag, &fragment_shader) != VK_SUCCESS)
        goto cleanup_swapchain_images;

    VkShaderModule vertex_shader;
    if (create_shader_module(device, SHADER_CONTENTS_shader_vert_len,
                             (char *)SHADER_CONTENTS_shader_vert, &vertex_shader) != VK_SUCCESS)
        goto cleanup_frag_shader;

    VkPipelineLayout pipeline_layout;
    if (create_graphics_pipeline_layout(device, &pipeline_layout) != VK_SUCCESS)
        goto cleanup_vertex_shader;

    VkRenderPass render_pass;
    if (create_render_pass(device, swapchain_details, &render_pass) != VK_SUCCESS)
        goto cleanup_pipeline_layout;

    VkPipeline pipeline;
    if (create_graphics_pipeline(device, fragment_shader, vertex_shader,
                                 swapchain_details.surface_extent, pipeline_layout, render_pass,
                                 &pipeline) != VK_SUCCESS)
        goto cleanup_render_pass;

    Array framebuffers;
    if (create_framebuffers(allocator, device, image_views, render_pass,
                            swapchain_details.surface_extent, &framebuffers) != VK_SUCCESS)
        goto cleanup_pipeline;

    VkCommandPool command_pool;
    if (create_command_pool(device, queue_families, &command_pool) != VK_SUCCESS)
        goto cleanup_framebuffers;

    VkCommandBuffer command_buffer;
    if (create_command_buffer(device, command_pool, &command_buffer) != VK_SUCCESS)
        goto cleanup_command_pool;

    SyncObjects sync_objects;
    if (create_sync_objects(device, &sync_objects) != VK_SUCCESS)
        goto cleanup_command_pool;

    RendererState renderer = {
        .swapchain_details = swapchain_details,
        .swapchain_images = swapchain_images,
        .swapchain_views = image_views,
        .device = device,
        .device_queue = device_queue,
        .physical_device = physical_device_details,
        .surface = surface,
        .instance = instance,

        .render_pass = render_pass,
        .pipeline_layout = pipeline_layout,
        .pipeline = pipeline,

        .framebuffers = framebuffers,

        .command_pool = command_pool,
        .command_buffer = command_buffer,
        .sync_objects = sync_objects,

        .frag = fragment_shader,
        .vert = vertex_shader,
    };

    *out = renderer;

    return true;

cleanup_command_pool:
    vkDestroyCommandPool(device, command_pool, NULL);

cleanup_framebuffers:
    destroy_framebuffers(allocator, device, &framebuffers);

cleanup_pipeline:
    vkDestroyPipeline(device, pipeline, NULL);

cleanup_render_pass:
    vkDestroyRenderPass(device, render_pass, NULL);

cleanup_pipeline_layout:
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);

cleanup_vertex_shader:
    vkDestroyShaderModule(device, vertex_shader, NULL);

cleanup_frag_shader:
    vkDestroyShaderModule(device, fragment_shader, NULL);

cleanup_swapchain_images:
    destroy_image_views(allocator, device, &image_views);

cleanup_swapchain:
    vkDestroySwapchainKHR(device, swapchain_details.swapchain, NULL);
    free_swapchain_support_details(allocator, &swapchain_details.support);
    array_free(&swapchain_images, allocator);

cleanup_device:
    vkDestroyDevice(device, NULL);

fail:
    return false;
}

void renderer_destroy(Allocator allocator, RendererState *renderer) {
    VkDevice device = renderer->device;

    vkDestroyCommandPool(device, renderer->command_pool, NULL);

    destroy_framebuffers(allocator, device, &renderer->framebuffers);

    vkDestroyPipeline(device, renderer->pipeline, NULL);
    vkDestroyRenderPass(device, renderer->render_pass, NULL);
    vkDestroyPipelineLayout(device, renderer->pipeline_layout, NULL);

    vkDestroyShaderModule(device, renderer->frag, NULL);
    vkDestroyShaderModule(device, renderer->vert, NULL);

    destroy_image_views(allocator, device, &renderer->swapchain_views);

    vkDestroySwapchainKHR(device, renderer->swapchain_details.swapchain, NULL);
    free_swapchain_support_details(allocator, &renderer->swapchain_details.support);
    array_free(&renderer->swapchain_images, allocator);

    vkDestroyDevice(device, NULL);
}
