#define GLFW_INCLUDE_VULKAN
#define VK_NO_PROTOTYPES

#include <GLFW/glfw3.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <volk.h>

const uint32_t WIN_WIDTH = 800;
const uint32_t WIN_HEIGHT = 600;

int start_client() {
    assert(volkInitialize() == VK_SUCCESS);

    if (glfwInit() != GLFW_TRUE) {
        const char *description;
        int code = glfwGetError(&description);
        printf("GLFW initialization failed!\n");
        printf("Error %d: %s\n", code,
               description ? description : "Unknown error");
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow *window =
        glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Vulkan window", NULL, NULL);

    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount = 0;

    VkInstance instance;
    assert(vkCreateInstance(&createInfo, NULL, &instance) == VK_SUCCESS);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
    printf("Extension count: %d\n", extensionCount);

    VkExtensionProperties *extensionList =
        malloc(sizeof(VkExtensionProperties) * extensionCount);
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount,
                                           extensionList);

    for (int i = 0; i < extensionCount; i++) {
        printf("\tExtension name: %s\n", extensionList[i].extensionName);
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
