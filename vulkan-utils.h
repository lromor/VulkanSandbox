
#ifndef _VULKAN_UTILS_H
#define _VULKAN_UTILS_H

#include <iostream>
#include <vulkan/vulkan.h>

#define VK_CHECK_RESULT(f) { \
  VkResult res = (f); \
  if (res != VK_SUCCESS) { \
    fprintf(stderr, "%s:%d: VkResult != VK_SUCCESS\n", __FILE__, __LINE__); \
    exit(1); \
  } \
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
    uint64_t obj, size_t location, int32_t code, const char* layerPrefix,
    const char* msg, void* userData) {
  std::cerr << "validation layer: " << msg << std::endl;
  return VK_FALSE;
}

#endif
