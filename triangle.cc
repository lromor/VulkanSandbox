
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <assert.h>
#include <fstream>
#include <limits>

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

  void LoadShaderModule(const char *path, VkShaderModule *module);

private:

  const char *application_name_ = "Triangle";

  // Shader stuff
  VkShaderModule shader_module_;

  // Vulkan stuff
  VkDevice device_;
  VkPhysicalDevice physical_device_;
  VkInstance instance_;
  VkSurfaceKHR surface_;
  VkSwapchainKHR swap_chain_;
  VkCommandPool command_pool_;
  std::vector<VkCommandBuffer> command_buffers_;

  VkSemaphore image_available_semaphore_;
  VkSemaphore render_finished_semaphore_;

  VkQueue graphics_queue_;
  VkQueue present_queue_;
  VkDebugReportCallbackEXT callback_;
  std::vector<VkImage> swap_chain_images_;
  VkFormat swapChainImageFormat;
  std::vector<VkImageView> swap_chain_image_views_;

  VkPipelineLayout pipeline_layout_;
  VkRenderPass render_pass_;
  VkPipeline graphics_pipeline_;
  std::vector<VkFramebuffer> swap_chain_frame_buffers_;

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

void Triangle::LoadShaderModule(const char *path, VkShaderModule *module) {
  // Open the file
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  size_t fileSize = (size_t) file.tellg();
  file.seekg(0);

  std::vector<char> buffer(fileSize);
  file.read(buffer.data(), fileSize);
  file.close();

  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = buffer.size();
  createInfo.pCode = (uint32_t*) buffer.data();

  VK_CHECK_RESULT(
    vkCreateShaderModule(device_, &createInfo, NULL, module));
}

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

  // Let's create the shaders
  VkShaderModule vertex;
  VkShaderModule fragment;

  LoadShaderModule("triangle.vert.spv", &vertex);
  LoadShaderModule("triangle.frag.spv", &fragment);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertex;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragment;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  // we need to finish the pipeline now :S
  // Vertex index

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = NULL; // Optional
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = NULL; // Optional

  // Input assembly
  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float) 800;
  viewport.height = (float) 600;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = extent;

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  // Rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer.depthBiasClamp = 0.0f; // Optional
  rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

  // Multisampling
  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f; // Optional
  multisampling.pSampleMask = nullptr; /// Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable = VK_FALSE; // Optional

  // Depth and stencil testing
  // None

  // Color blending
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f; // Optional
  colorBlending.blendConstants[1] = 0.0f; // Optional
  colorBlending.blendConstants[2] = 0.0f; // Optional
  colorBlending.blendConstants[3] = 0.0f; // Optional

  // Dynamic state
  // VkDynamicState dynamicStates[] = {
  //     VK_DYNAMIC_STATE_VIEWPORT,
  //     VK_DYNAMIC_STATE_LINE_WIDTH
  // };
  //
  // VkPipelineDynamicStateCreateInfo dynamicState = {};
  // dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  // dynamicState.dynamicStateCount = 2;
  // dynamicState.pDynamicStates = dynamicStates;

  // Create pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0; // Optional
  pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
  pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
  pipelineLayoutInfo.pPushConstantRanges = 0; // Optional

  VK_CHECK_RESULT(
    vkCreatePipelineLayout(device_, &pipelineLayoutInfo, NULL, &pipeline_layout_));

  // Render passes
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = surfaceFormat.format;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  // Create the render pass
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  VK_CHECK_RESULT(
    vkCreateRenderPass(device_, &renderPassInfo, NULL, &render_pass_));

  // Create the pipeline (FINALLY)
  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = NULL; // Optional
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = NULL; // Optional
  pipelineInfo.layout = pipeline_layout_;
  pipelineInfo.renderPass = render_pass_;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex = -1; // Optional
  VK_CHECK_RESULT(
    vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphics_pipeline_));

  // Framebuffers

  swap_chain_frame_buffers_.resize(swap_chain_image_views_.size());
  for (size_t i = 0; i < swap_chain_image_views_.size(); i++) {
    VkImageView attachments[] = {
        swap_chain_image_views_[i]
    };
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = render_pass_;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;

    VK_CHECK_RESULT(
      vkCreateFramebuffer(device_, &framebufferInfo, NULL, &swap_chain_frame_buffers_.data()[i]));
  }

  // // COMMAND POOLS (YEEEEE)
  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = 0;
  poolInfo.flags = 0; // Optional

  VK_CHECK_RESULT(
    vkCreateCommandPool(device_, &poolInfo, NULL, &command_pool_));

  command_buffers_.resize(swap_chain_frame_buffers_.size());
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = command_pool_;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t) command_buffers_.size();
  VK_CHECK_RESULT(
    vkAllocateCommandBuffers(device_, &allocInfo, command_buffers_.data()));

  for (size_t i = 0; i < command_buffers_.size(); i++) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = NULL; // Optional

    vkBeginCommandBuffer(command_buffers_[i], &beginInfo);

    // OK, we are recording now let's do something
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = render_pass_;
    renderPassInfo.framebuffer = swap_chain_frame_buffers_[i];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;
    VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    vkCmdBeginRenderPass(command_buffers_[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);
    vkCmdDraw(command_buffers_[i], 3, 1, 0, 0);
    vkCmdEndRenderPass(command_buffers_[i]);
    VK_CHECK_RESULT(vkEndCommandBuffer(command_buffers_[i]));
  }
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VK_CHECK_RESULT(
    vkCreateSemaphore(device_, &semaphoreInfo, NULL, &image_available_semaphore_));
  VK_CHECK_RESULT(
    vkCreateSemaphore(device_, &semaphoreInfo, NULL, &render_finished_semaphore_));

  uint32_t imageIndex;
  vkAcquireNextImageKHR(device_, swap_chain_, std::numeric_limits<uint64_t>::max(), image_available_semaphore_, VK_NULL_HANDLE, &imageIndex);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {image_available_semaphore_};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &command_buffers_[imageIndex];
  VkSemaphore signalSemaphores[] = {render_finished_semaphore_};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  VK_CHECK_RESULT(
    vkQueueSubmit(graphics_queue_, 1, &submitInfo, VK_NULL_HANDLE));

  // WTF STUFF
  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;

  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;

  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  // PRESENTATION

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swap_chain_};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = NULL; // Optional
  vkQueuePresentKHR(present_queue_, &presentInfo);
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
