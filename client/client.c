#define GLFW_INCLUDE_VULKAN
#define VK_NO_PROTOTYPES

#include <GLFW/glfw3.h>

#include <stdio.h>
#include <volk.h>

int start_client() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow *window =
      glfwCreateWindow(800, 600, "Vulkan window", NULL, NULL);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
