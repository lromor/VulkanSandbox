
#include <iostream>

#include <xcb/xcb.h>
#include <vulkan/vulkan.h>

#define VK_CHECK_RESULT(f) { \
  VkResult res = (f); \
  if (res != VK_SUCCESS) { \
    fprintf(stderr, "VkResult != VK_SUCCESS\n"); \
    exit(1); \
  } \
}

class VulkanCore {
public:
  VulkanCore() {}
  virtual ~VulkanCore() {}
  virtual void InitVulkan() {}
};
