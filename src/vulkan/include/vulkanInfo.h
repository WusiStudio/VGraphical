#pragma once
#ifndef __VULKAN_INFO_H__
#define __VULKAN_INFO_H__

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class vulkanInfo
{
public:

    static vulkanInfo instance;

    bool validate;
    bool use_break;
    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
    VkDebugReportCallbackEXT msg_callback;
    PFN_vkDebugReportMessageEXT DebugReportMessage;

    VkInstance inst;
    VkPhysicalDevice gpu;

    uint32_t enabled_layer_count;
    uint32_t enabled_extension_count;
    const char * extension_names[64];
    const char * enabled_layers[64];

};

#endif //__VULKAN_INFO_H__