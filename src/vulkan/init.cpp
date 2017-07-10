#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "VGraphical.h"
#include "log.hpp"

namespace ROOT_SPACE
{
    bool initGraphical(void)
    {
        if (!glfwVulkanSupported())
        {
            LOG.error("GLFW failed to find the Vulkan loader.\nExiting ...");
            return true;
        }

        LOG.info("hello vulkan demo");

        return false;
    }
}