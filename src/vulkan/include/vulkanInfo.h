#pragma once
#ifndef __VULKAN_INFO_H__
#define __VULKAN_INFO_H__

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <map>

class vulkanInfo
{
public:

    static vulkanInfo instance;

    std::map< GLFWwindow *, VkSurfaceKHR > surfaces;

    bool validate;
    bool use_break;
    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
    VkDebugReportCallbackEXT msg_callback;
    PFN_vkDebugReportMessageEXT DebugReportMessage;

    VkInstance inst;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkQueue queue;
    VkPhysicalDeviceProperties gpu_props;
    VkPhysicalDeviceFeatures gpu_features;
    VkQueueFamilyProperties *queue_props;
    uint32_t graphics_queue_node_index;

    PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;

    uint32_t enabled_layer_count;
    uint32_t enabled_extension_count;
    const char * extension_names[64];
    const char * enabled_layers[64];

    VkFormat format;
    VkColorSpaceKHR color_space;

    uint32_t queue_count;
};

#endif //__VULKAN_INFO_H__