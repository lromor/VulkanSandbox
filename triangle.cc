
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <assert.h>

#include <xcb/xcb.h>
#include <vulkan/vulkan.h>

#include "vulkan-core.h"


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
    uint64_t obj, size_t location, int32_t code, const char* layerPrefix,
    const char* msg, void* userData) {
  std::cerr << "validation layer: " << msg << std::endl;
  return VK_FALSE;
}

class Triangle : public VulkanCore {
public:
  Triangle() {}

  void CreateWindow(uint32_t x, uint32_t y, uint16_t width, uint16_t height);
  void InitVulkan() {
    InitVulkanInstance();
    InitVulkanPhysicalDevice();
    CreateSurface();
  }
  void Loop();
private:

  const char *application_name_ = "Triangle";

  // Vulkan stuff
  VkDevice device_;
  VkPhysicalDevice physical_device_;
  VkInstance instance_;
  VkSurfaceKHR surface_;
  VkSwapchainKHR swap_chain_;

  VkQueue graphics_queue_;
  VkQueue present_queue_;
  VkDebugReportCallbackEXT callback_;
  std::vector<VkImage> swap_chain_images_;
  VkFormat swapChainImageFormat;
  std::vector<VkImageView> swap_chain_image_views_;

  void InitVulkanInstance();
  void InitVulkanPhysicalDevice();
  void CreateSurface();

  const std::vector<const char*> validation_layers_ = {
    "VK_LAYER_LUNARG_standard_validation"
  };

  // Xcb stuff
  uint16_t width_;
  uint16_t height_;
  xcb_connection_t *connection_;
  xcb_window_t window_;
  xcb_screen_t *screen_;
  xcb_intern_atom_reply_t *atom_wm_delete_window_;
};

void Triangle::CreateSurface() {
  VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
  surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
  surfaceCreateInfo.connection = connection_;
  surfaceCreateInfo.window = window_;
  VK_CHECK_RESULT(
    vkCreateXcbSurfaceKHR(instance_, &surfaceCreateInfo, NULL, &surface_));


  uint32_t queueCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queueCount, NULL);

  std::cout << "Queue count " << queueCount << std::endl;

  uint32_t graphics_queue_family_index = UINT32_MAX;
  uint32_t present_queue_family_index = UINT32_MAX;

  std::vector<VkBool32> queue_surface_support(queueCount);

  for (uint32_t i = 0; i < queueCount; ++i) {
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_, i,
                                         surface_,
                                         &queue_surface_support[i]);
    // If there is queue that supports both graphics and present - prefer it
    if(queue_surface_support[i] ) {
      graphics_queue_family_index = i;
      present_queue_family_index = i;
    }
  }

  std::cout <<  "Graphics index: " << graphics_queue_family_index << std::endl;
  std::cout <<  "Graphics index: " << present_queue_family_index << std::endl;


  vkGetDeviceQueue(device_, 0, 0, &present_queue_);

  VkSurfaceCapabilitiesKHR details;
  VK_CHECK_RESULT(
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &details));


  // FORMAT OF THE FORMAT
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &formatCount, NULL);
  assert(formatCount != 0);
  std::vector<VkSurfaceFormatKHR> formats(formatCount);

  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &formatCount, formats.data());

  // PRESENT FORMAT
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &presentModeCount, NULL);
  assert(presentModeCount != 0);
  std::vector<VkPresentModeKHR> presentModes(presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &presentModeCount, presentModes.data());

  // We skip checks, as always...
  VkSurfaceFormatKHR surfaceFormat = formats[0];
  VkPresentModeKHR presentMode = presentModes[0];
  VkExtent2D extent = {800, 600};

  uint32_t imageCount = 2;

  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface_;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.queueFamilyIndexCount = 0; // Optional
  createInfo.pQueueFamilyIndices = NULL; // Optional

  createInfo.preTransform = details.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  createInfo.oldSwapchain = VK_NULL_HANDLE;

  VK_CHECK_RESULT(
    vkCreateSwapchainKHR(device_, &createInfo, NULL, &swap_chain_));


  vkGetSwapchainImagesKHR(device_, swap_chain_, &imageCount, NULL);
  swap_chain_images_.resize(imageCount);
  vkGetSwapchainImagesKHR(device_, swap_chain_, &imageCount, swap_chain_images_.data());

  swap_chain_image_views_.resize(imageCount);

  VkImageViewCreateInfo createInfo2 = {};

  for (uint32_t i = 0; i < swap_chain_images_.size(); i++) {
    createInfo2.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo2.image = swap_chain_images_[i];
    createInfo2.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo2.format = surfaceFormat.format;
    createInfo2.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo2.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo2.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo2.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo2.subresourceRange.baseMipLevel = 0;
    createInfo2.subresourceRange.levelCount = 1;
    createInfo2.subresourceRange.baseArrayLayer = 0;
    createInfo2.subresourceRange.layerCount = 1;
    VK_CHECK_RESULT(
      vkCreateImageView(device_, &createInfo2, NULL, &swap_chain_image_views_[i]));
  }


}

void Triangle::InitVulkanPhysicalDevice() {
  // Get a physical device now
  uint32_t deviceCount = 0;
  VK_CHECK_RESULT(
    vkEnumeratePhysicalDevices(instance_, &deviceCount, NULL));

  std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
  VK_CHECK_RESULT(
    vkEnumeratePhysicalDevices(instance_, &deviceCount, physicalDevices.data()));

  physical_device_ = physicalDevices[0];

  // Let's get the queue family properties of the vulkan device
  uint32_t pqf_count_;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &pqf_count_, NULL);

  std::vector<VkQueueFamilyProperties> physicalDevicesQProperties(pqf_count_);
  vkGetPhysicalDeviceQueueFamilyProperties(
    physical_device_, &pqf_count_, physicalDevicesQProperties.data());

  // Now the logical device
  float priorities[] = {1.0f};
  VkDeviceQueueCreateInfo queueInfo{};
  queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueInfo.pNext = NULL;
  queueInfo.flags = 0;
  queueInfo.queueFamilyIndex = 0;
  queueInfo.queueCount = 1;
  queueInfo.pQueuePriorities = &priorities[0];

  std::vector<const char *> enabledExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo deviceInfo{};
  deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceInfo.pNext = NULL;
  deviceInfo.flags = 0;
  deviceInfo.enabledLayerCount = validation_layers_.size();
  deviceInfo.ppEnabledLayerNames = validation_layers_.data();
  deviceInfo.queueCreateInfoCount = 1;
  deviceInfo.pQueueCreateInfos = &queueInfo;
  deviceInfo.enabledExtensionCount = enabledExtensions.size();
  deviceInfo.ppEnabledExtensionNames = enabledExtensions.data();
  deviceInfo.pEnabledFeatures = NULL;

  VK_CHECK_RESULT(vkCreateDevice(physical_device_, &deviceInfo, NULL, &device_));
  VkPhysicalDeviceProperties physicalProperties = {};

  for (uint32_t i = 0; i < deviceCount; i++) {
    vkGetPhysicalDeviceProperties(physicalDevices[i], &physicalProperties);
    fprintf(stdout, "Device Name:    %s\n", physicalProperties.deviceName);
    fprintf(stdout, "Device Type:    %d\n", physicalProperties.deviceType);
    fprintf(stdout, "Driver Version: %d\n", physicalProperties.driverVersion);
    fprintf(stdout, "API Version:    %d.%d.%d\n",
            VK_VERSION_MAJOR(physicalProperties.apiVersion),
            VK_VERSION_MINOR(physicalProperties.apiVersion),
            VK_VERSION_PATCH(physicalProperties.apiVersion));
  }
  vkGetDeviceQueue(device_, 0, 0, &graphics_queue_);
}

void Triangle::InitVulkanInstance() {
  // First step: create vulkan instance
  VkInstanceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.pApplicationInfo = NULL;

  // // NOTE: Let's query vulkan for supported layers
  // uint32_t layer_count = 0;
  // vkEnumerateInstanceLayerProperties(&layer_count, NULL);
  // std::vector<VkLayerProperties> availableLayers(layer_count);
  // vkEnumerateInstanceLayerProperties(&layer_count, availableLayers.data());
  //
  // for (const auto& avail_layer : availableLayers) {
  //   std::cout << avail_layer.layerName << std::endl;
  // }

  create_info.enabledLayerCount = validation_layers_.size();
  create_info.ppEnabledLayerNames = validation_layers_.data();

  // // NOTE: Let's query Vulkan to get the enabled extensions
  // uint32_t extension_count = 0;
  // vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
  // std::vector<VkExtensionProperties> extensions(extension_count);
  // vkEnumerateInstanceExtensionProperties(NULL, &extension_count,
  //                                        extensions.data());
  //
  // std::cout << "Available extensions:" << std::endl;
  // for (const auto& extension : extensions) {
  //   std::cout << "\t" << extension.extensionName << std::endl;
  // }
  //
  // // Check the xcb surface support is present
  // bool xcb_ext_found = false;
  // for (const auto& extension : extensions) {
  //   if (strcmp(extension.extensionName, VK_KHR_XCB_SURFACE_EXTENSION_NAME) == 0)
  //     xcb_ext_found = true;
  // }

  // We pick the xcb required extension
  std::vector<const char *> enabledExtensions = \
    {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_XCB_SURFACE_EXTENSION_NAME,
     VK_EXT_DEBUG_REPORT_EXTENSION_NAME};

  create_info.enabledExtensionCount = enabledExtensions.size();
  create_info.ppEnabledExtensionNames = enabledExtensions.data();

  VK_CHECK_RESULT(vkCreateInstance(&create_info, NULL, &instance_));

  // Let's add the debug callback function
  VkDebugReportCallbackCreateInfoEXT createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
  createInfo.pfnCallback = debugCallback;

  // Get the vkCreateDebugReportCallbackEXT function address
  auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)
    vkGetInstanceProcAddr(instance_, "vkCreateDebugReportCallbackEXT");

  VK_CHECK_RESULT(
    vkCreateDebugReportCallbackEXT(instance_, &createInfo, NULL, &callback_));
}

void Triangle::Loop() {
  xcb_generic_event_t  *event;
  for (;;) {

    if ( (event = xcb_poll_for_event(connection_)) ) {
      switch (event->response_type & ~0x80) {
        case XCB_EXPOSE: {
          // Stuff should go here?
          break;
        }
        case XCB_CLIENT_MESSAGE: {
          if ((*(xcb_client_message_event_t *)event).data.data32[0] ==
          (*atom_wm_delete_window_).atom) {
            exit(0);
          }
        }
            break;
        case XCB_KEY_RELEASE: {
          xcb_key_release_event_t *kr = (xcb_key_release_event_t *)event;

          switch (kr->detail) {
              // Esc
              case 9: {
                  free (event);
                  xcb_disconnect (connection_);
                  exit(0);
              }
          }
          free (event);
        }
      }

    }
  }

}

void Triangle::CreateWindow(uint32_t x, uint32_t y,
                            uint16_t width, uint16_t height) {
  // Chose the DISPLAY
  width_ = width;
  height_ = height;

  int screenp = 0;
  connection_ = xcb_connect(NULL, &screenp);

  if (xcb_connection_has_error(connection_))
    fprintf(stderr, "Problems on connectin through XCB");

  xcb_screen_iterator_t iter =
      xcb_setup_roots_iterator(xcb_get_setup(connection_));

  for (int s = screenp; s > 0; s--) xcb_screen_next(&iter);

  screen_ = iter.data;
  window_ = xcb_generate_id(connection_);

  uint32_t event_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  uint32_t value_list[2];
  value_list[0] = screen_->white_pixel;
  value_list[1] = XCB_EVENT_MASK_KEY_RELEASE |
              XCB_EVENT_MASK_BUTTON_PRESS |
              XCB_EVENT_MASK_EXPOSURE |
              XCB_EVENT_MASK_POINTER_MOTION;

  xcb_create_window(connection_,                    // Connection
                    XCB_COPY_FROM_PARENT,           // depth (same as root)
                    window_,                        // window Id
                    screen_->root,                  // parent window
                    0, 0,                           // x, y
                    width, height,                  // width, height
                    0,                              // border_width
                    XCB_WINDOW_CLASS_INPUT_OUTPUT,  // class
                    screen_->root_visual,           // visual
                    event_mask, value_list);        // masks, not used yet

  // Required to close the program when you close the window
  xcb_intern_atom_cookie_t cookie =
      xcb_intern_atom(connection_, 1, 12, "WM_PROTOCOLS");
  xcb_intern_atom_reply_t *reply =
      xcb_intern_atom_reply(connection_, cookie, 0);

  xcb_intern_atom_cookie_t cookie2 =
      xcb_intern_atom(connection_, 0, 16, "WM_DELETE_WINDOW");
  atom_wm_delete_window_ =
      xcb_intern_atom_reply(connection_, cookie2, 0);

  xcb_change_property(connection_, XCB_PROP_MODE_REPLACE, window_,
                      (*reply).atom, 4, 32, 1,
                      &(*atom_wm_delete_window_).atom);
  free(reply);

  xcb_map_window(connection_, window_);

  const uint32_t coords[] = {x, y};
  xcb_configure_window(connection_, window_,
                       XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
  xcb_flush(connection_);
}

int main (int argc, char *argv[]) {
  Triangle a = Triangle();

  // We've got a window
  a.CreateWindow(300, 200, 800, 600);

  // Let's instance Vulkan now
  a.InitVulkan();

  a.Loop();

  std::cout << "Bye!" << std::endl;
  return 0;
}
