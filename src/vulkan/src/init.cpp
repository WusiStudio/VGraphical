#include "vulkanInfo.h"
#include "VGraphical.h"
#include "log.hpp"
#include <cassert>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

vulkanInfo vulkanInfo::instance;

namespace ROOT_SPACE
{

    #define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                               \
    {                                                                          \
        vulInfo.fp##entrypoint =                                                 \
            (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint); \
        if (vulInfo.fp##entrypoint == nullptr) {                                    \
            LOG.error("vkGetInstanceProcAddr Failure: vkGetInstanceProcAddr failed to find vk" #entrypoint ); \
            return true;                                                        \
        }                                                                      \
    }

    #define GET_DEVICE_PROC_ADDR(dev, entrypoint)                                  \
    {                                                                          \
        vulInfo.fp##entrypoint =                                                 \
            (PFN_vk##entrypoint)vkGetDeviceProcAddr(dev, "vk" #entrypoint);    \
        if (vulInfo.fp##entrypoint == nullptr) {                                    \
            LOG.error("vkGetDeviceProcAddr Failure: vkGetDeviceProcAddr failed to find vk" #entrypoint );                           \
        }                                                                      \
    }

    /*
    * Return 1 (true) if all layer names specified in check_names
    * can be found in given layer properties.
    */
    VkBool32 vulkan_check_layers(uint32_t check_count, const char **check_names,
                                    uint32_t layer_count,
                                    VkLayerProperties *layers) 
    {
        uint32_t i, j;
        for (i = 0; i < check_count; i++) {
            VkBool32 found = 0;
            for (j = 0; j < layer_count; j++) {
                if (!strcmp(check_names[i], layers[j].layerName)) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                fprintf(stderr, "Cannot find layer: %s\n", check_names[i]);
                return 0;
            }
        }
        return 1;
    }
    static int validation_error = 0;

    VKAPI_ATTR VkBool32 VKAPI_CALL
    BreakCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
                uint64_t srcObject, size_t location, int32_t msgCode,
                const char *pLayerPrefix, const char *pMsg,
                void *pUserData) {
        #ifdef _WIN32
            DebugBreak();
        #else
            raise(SIGTRAP);
        #endif

        return false;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL
    dbgFunc(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
        uint64_t srcObject, size_t location, int32_t msgCode,
        const char *pLayerPrefix, const char *pMsg, void *pUserData) {

        validation_error = 1;

        if( msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT )
        {
            LOG.error( "{0} Code {1} : {2}", pLayerPrefix, msgCode, pMsg );
        } else if ( msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT ) 
        {
            LOG.warning( "{0} Code {1} : {2}", pLayerPrefix, msgCode, pMsg );
        }

        /*
        * false indicates that layer should not bail-out of an
        * API call that had validation failures. This may mean that the
        * app dies inside the driver due to invalid parameter(s).
        * That's what would happen without validation layers, so we'll
        * keep that behavior here.
        */
        return false;
    }

    bool VGraphical::initGraphical(void)
    {

        glfwSetErrorCallback( VGraphical::__glfw_error_callback );

        if (!glfwInit()) 
        {
            LOG.error("Cannot initialize GLFW.\nExiting ...");
            return true;
        }

        if (!glfwVulkanSupported())
        {
            LOG.error("GLFW failed to find the Vulkan loader.\nExiting ...");
            return true;
        }

        VkResult err;

        vulkanInfo & vulInfo = vulkanInfo::instance;
		vulInfo.validate = true;

        vulInfo.enabled_layer_count = 0;
        vulInfo.enabled_extension_count = 0;

        const char **instance_validation_layers = nullptr;


        char *instance_validation_layers_alt1[] = {
            "VK_LAYER_LUNARG_standard_validation"
        };

        char *instance_validation_layers_alt2[] = {
            "VK_LAYER_GOOGLE_threading",       "VK_LAYER_LUNARG_parameter_validation",
            "VK_LAYER_LUNARG_object_tracker",  "VK_LAYER_LUNARG_image",
            "VK_LAYER_LUNARG_core_validation", "VK_LAYER_LUNARG_swapchain",
            "VK_LAYER_GOOGLE_unique_objects"
        };
       
        /* Look for validation layers */
        VkBool32 validation_found = 0;
        if( vulInfo.validate )
        {
            //get instance layer count
            uint32_t instance_layer_count = 0;
            err = vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr);
            assert(!err);

            instance_validation_layers = (const char **)instance_validation_layers_alt1;

            
            if (instance_layer_count > 0) 
            {
                //check instance layer count
                VkLayerProperties * instance_layers =
                    (VkLayerProperties *)malloc(sizeof (VkLayerProperties) * instance_layer_count);
                err = vkEnumerateInstanceLayerProperties(&instance_layer_count,
                        instance_layers);
                assert(!err);

                //check 
                validation_found = vulkan_check_layers(
                    ARRAY_SIZE(instance_validation_layers_alt1),
                    instance_validation_layers, instance_layer_count,
                    instance_layers);
                if (validation_found) 
                {
                    vulInfo.enabled_layer_count = ARRAY_SIZE(instance_validation_layers_alt1);
                    vulInfo.enabled_layers[0] = instance_validation_layers[0];
                } else 
                {
                    // use alternative set of validation layers
                    instance_validation_layers = (const char **)instance_validation_layers_alt2;
                    vulInfo.enabled_layer_count = ARRAY_SIZE(instance_validation_layers_alt2);
                    validation_found = vulkan_check_layers(
                        ARRAY_SIZE(instance_validation_layers_alt2),
                        instance_validation_layers, instance_layer_count,
                        instance_layers);
                    uint32_t validation_layer_count =
                        ARRAY_SIZE(instance_validation_layers_alt2);
                    for (uint32_t i = 0; i < validation_layer_count; i++) 
                    {
                        vulInfo.enabled_layers[i] = instance_validation_layers[i];
                    }
                }
                free(instance_layers);
            }

            if(!validation_found)
            {
				LOG.error ( "vkEnumerateInstanceLayerProperties failed to find "
					"required validation layer.\n\n"
					"Please look at the Getting Started guide for additional "
					"information." );
                return true;
            }
        }

        /* Look for instance extensions */
        uint32_t required_extension_count = 0;
        const char **required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);
        if (!required_extensions)
        {
            LOG.error("glfwGetRequiredInstanceExtensions failed to find the "
                 "platform surface extensions.\n\nDo you have a compatible "
                 "Vulkan installable client driver (ICD) installed?\nPlease "
                 "look at the Getting Started guide for additional "
                 "information.");
            return true;
        }

        for (uint32_t i = 0; i < required_extension_count; i++)
        {
            vulInfo.extension_names[vulInfo.enabled_extension_count++] = required_extensions[i];
            assert( vulInfo.enabled_extension_count < 64 );
        }

        uint32_t instance_extension_count = 0;
        err = vkEnumerateInstanceExtensionProperties( nullptr, &instance_extension_count, nullptr );
        assert( !err );

        if ( instance_extension_count > 0 ) 
        {
            VkExtensionProperties *instance_extensions = (VkExtensionProperties *)malloc( sizeof( VkExtensionProperties ) * instance_extension_count);
            err = vkEnumerateInstanceExtensionProperties( nullptr, &instance_extension_count, instance_extensions );
            assert( !err );

            for (uint32_t i = 0; i < instance_extension_count; i++) 
            {
                if ( !strcmp( VK_EXT_DEBUG_REPORT_EXTENSION_NAME, instance_extensions[i].extensionName ) ) 
                {
                    if ( vulInfo.validate ) 
                    {
                        vulInfo.extension_names[vulInfo.enabled_extension_count++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
                    }
                }
                assert( vulInfo.enabled_extension_count < 64 );
            }

            free( instance_extensions );
        }

		VkApplicationInfo app;
		app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app.pNext = nullptr;
		app.pApplicationName = "haha";
		app.applicationVersion = 0;
		app.pEngineName = "ws";
		app.engineVersion = 0;
		app.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo inst_info;
		inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		inst_info.pNext = nullptr;
        inst_info.flags = 0;
		inst_info.pApplicationInfo = &app;
		inst_info.enabledLayerCount = vulInfo.enabled_layer_count;
		inst_info.ppEnabledLayerNames = (const char *const *)instance_validation_layers;
		inst_info.enabledExtensionCount = vulInfo.enabled_extension_count;
		inst_info.ppEnabledExtensionNames = (const char *const *)vulInfo.extension_names;

        uint32_t gpu_count;

        err = vkCreateInstance( &inst_info, nullptr, &vulInfo.inst );
        if (err == VK_ERROR_INCOMPATIBLE_DRIVER) 
        {
            LOG.error( "vkCreateInstance Failure: Cannot find a compatible Vulkan installable client driver "
                    "(ICD).\n\nPlease look at the Getting Started guide for "
                    "additional information." );
            return true;
        } else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) 
        {
            LOG.error( "vkCreateInstance Failure: Cannot find a specified extension library"
                    ".\nMake sure your layers path is set appropriately" );
            return true;
        } else if (err) 
        {
            LOG.error( "vkCreateInstance Failure: vkCreateInstance failed.\n\nDo you have a compatible Vulkan "
                    "installable client driver (ICD) installed?\nPlease look at "
                    "the Getting Started guide for additional information." );
            return true;
        }

        /* Make initial call to query gpu_count, then second call for gpu info*/
        err = vkEnumeratePhysicalDevices( vulInfo.inst, &gpu_count, nullptr );
        assert( !err && gpu_count > 0 );

        if ( gpu_count > 0 ) 
        {
            VkPhysicalDevice *physical_devices = (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * gpu_count );
            err = vkEnumeratePhysicalDevices( vulInfo.inst, &gpu_count, physical_devices );
            assert(!err);

            /* For tri demo we just grab the first physical device */
            vulInfo.gpu = physical_devices[0];
            free(physical_devices);
        }else
        {
            LOG.error( "vkEnumeratePhysicalDevices reported zero accessible devices."
                 "\n\nDo you have a compatible Vulkan installable client"
                 " driver (ICD) installed?\nPlease look at the Getting Started"
                 " guide for additional information." );
            return true;
        }

        /* Look for device extensions */
        uint32_t device_extension_count = 0;
        VkBool32 swapchainExtFound = 0;
        vulInfo.enabled_extension_count = 0;

        err = vkEnumerateDeviceExtensionProperties( vulInfo.gpu, nullptr, &device_extension_count, nullptr );

        assert( !err );

        if (device_extension_count > 0) 
        {
            VkExtensionProperties *device_extensions = (VkExtensionProperties *)malloc (sizeof( VkExtensionProperties ) * device_extension_count );
            err = vkEnumerateDeviceExtensionProperties( vulInfo.gpu, nullptr, &device_extension_count, device_extensions );
            assert( !err );

            for (uint32_t i = 0; i < device_extension_count; i++) {
                if ( !strcmp( VK_KHR_SWAPCHAIN_EXTENSION_NAME, device_extensions[i].extensionName ) ) {
                    swapchainExtFound = 1;
                    vulInfo.extension_names[vulInfo.enabled_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
                }
                assert(vulInfo.enabled_extension_count < 64);
            }

            free(device_extensions);
        }

        if ( !swapchainExtFound ) {
            LOG.error("vkCreateInstance Failure: vkEnumerateDeviceExtensionProperties failed to find "
                    "the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
                    " extension.\n\nDo you have a compatible "
                    "Vulkan installable client driver (ICD) installed?\nPlease "
                    "look at the Getting Started guide for additional "
                    "information.\n");
        }

        if( vulInfo.validate )
        {
            vulInfo.CreateDebugReportCallback =
            (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
                vulInfo.inst, "vkCreateDebugReportCallbackEXT");
            if (!vulInfo.CreateDebugReportCallback) 
            {
                LOG.error( "vkGetProcAddr Failure: GetProcAddr: Unable to find vkCreateDebugReportCallbackEXT" );
                return true;
            }

            vulInfo.DestroyDebugReportCallback =
            (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
                vulInfo.inst, "vkDestroyDebugReportCallbackEXT");

            if (!vulInfo.DestroyDebugReportCallback) 
            {
                LOG.error( "vkGetProcAddr Failure: GetProcAddr: Unable to find vkDestroyDebugReportCallbackEXT" );
                return true;
            }

            vulInfo.DebugReportMessage =
            (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(
                vulInfo.inst, "vkDebugReportMessageEXT");
            if( !vulInfo.DebugReportMessage )
            {
                LOG.error("vkGetProcAddr Failure: GetProcAddr: Unable to find vkDebugReportMessageEXT" );
                return true;
            }

            VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
            dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
            dbgCreateInfo.flags =
                VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
            dbgCreateInfo.pfnCallback = vulInfo.use_break ? BreakCallback : dbgFunc;
            dbgCreateInfo.pUserData = &vulInfo;
            dbgCreateInfo.pNext = nullptr;

            err = vulInfo.CreateDebugReportCallback(vulInfo.inst, &dbgCreateInfo, nullptr,
                                              &vulInfo.msg_callback);
            switch (err) {
            case VK_SUCCESS:
                break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                    LOG.error( "CreateDebugReportCallback Failure: CreateDebugReportCallback: out of host memory" );
                    return true;
                break;
            default:
                    LOG.error( "CreateDebugReportCallback Failure CreateDebugReportCallback: unknown failure" );
                    return true;
                break;
            }
        }

        // Having these GIPA queries of device extension entry points both
        // BEFORE and AFTER vkCreateDevice is a good test for the loader
        GET_INSTANCE_PROC_ADDR( vulInfo.inst, GetPhysicalDeviceSurfaceCapabilitiesKHR );
        GET_INSTANCE_PROC_ADDR( vulInfo.inst, GetPhysicalDeviceSurfaceFormatsKHR );
        GET_INSTANCE_PROC_ADDR( vulInfo.inst, GetPhysicalDeviceSurfacePresentModesKHR );
        GET_INSTANCE_PROC_ADDR( vulInfo.inst, GetPhysicalDeviceSurfaceSupportKHR );

        vkGetPhysicalDeviceProperties( vulInfo.gpu, &vulInfo.gpu_props );
        
        // Query with nullptr data to get count
        vkGetPhysicalDeviceQueueFamilyProperties( vulInfo.gpu, &vulInfo.queue_count, nullptr );

        vulInfo.queue_props = ( VkQueueFamilyProperties * )malloc( vulInfo.queue_count * sizeof( VkQueueFamilyProperties ) );
        vkGetPhysicalDeviceQueueFamilyProperties(vulInfo.gpu, &vulInfo.queue_count, vulInfo.queue_props);

        assert(vulInfo.queue_count >= 1);

        vkGetPhysicalDeviceFeatures(vulInfo.gpu, &vulInfo.gpu_features);

        //init device
        float queue_priorities[1] = {0.0};
        VkDeviceQueueCreateInfo queue; 
        queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue.flags = 0;
        queue.pNext = nullptr;
        queue.queueFamilyIndex = vulInfo.graphics_queue_node_index;
        queue.queueCount = 1;
        queue.pQueuePriorities = queue_priorities;

        VkPhysicalDeviceFeatures features;
        memset(&features, 0, sizeof(features));
        if ( vulInfo.gpu_features.shaderClipDistance ) 
        {
            features.shaderClipDistance = VK_TRUE;
        }

        VkDeviceCreateInfo device;
        device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device.pNext = nullptr;
        device.flags = 0;
        device.queueCreateInfoCount = 1;
        device.pQueueCreateInfos = &queue;
        device.enabledLayerCount = 0;
        device.ppEnabledLayerNames = nullptr;
        device.enabledExtensionCount = vulInfo.enabled_extension_count;
        device.ppEnabledExtensionNames = (const char *const *)vulInfo.extension_names;
        device.pEnabledFeatures = &features;

        err = vkCreateDevice(vulInfo.gpu, &device, nullptr, &vulInfo.device);
        assert(!err);

        GET_DEVICE_PROC_ADDR(vulInfo.device, CreateSwapchainKHR);
        GET_DEVICE_PROC_ADDR(vulInfo.device, DestroySwapchainKHR);
        GET_DEVICE_PROC_ADDR(vulInfo.device, GetSwapchainImagesKHR);
        GET_DEVICE_PROC_ADDR(vulInfo.device, AcquireNextImageKHR);
        GET_DEVICE_PROC_ADDR(vulInfo.device, QueuePresentKHR);

        return false;
    }

    bool VGraphical::initWindow( window & p_window )
    {
        vulkanInfo & vulInfo = vulkanInfo::instance;
        VkResult U_ASSERT_ONLY err;

        // Create a WSI surface for the window:
        VkSurfaceKHR t_surface;
        vulInfo.surfaces[p_window._GLFW_WindowHandle()] = t_surface;
        glfwCreateWindowSurface( vulInfo.inst, p_window._GLFW_WindowHandle(), nullptr, &vulInfo.surfaces[p_window._GLFW_WindowHandle()]);

        // Iterate over each queue to learn whether it supports presenting:
        VkBool32 *supportsPresent = (VkBool32 *)malloc(vulInfo.queue_count * sizeof(VkBool32));

        for ( uint32_t i = 0; i < vulInfo.queue_count; i++) {
            vulInfo.fpGetPhysicalDeviceSurfaceSupportKHR(vulInfo.gpu, i, vulInfo.surfaces[p_window._GLFW_WindowHandle()], &supportsPresent[i]);
        }

        // Search for a graphics and a present queue in the array of queue
        // families, try to find one that supports both
        uint32_t graphicsQueueNodeIndex = UINT32_MAX;
        uint32_t presentQueueNodeIndex = UINT32_MAX;

        for ( uint32_t i = 0; i < vulInfo.queue_count; i++ ) 
        {
            if ( ( vulInfo.queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 ) 
            {
                if ( graphicsQueueNodeIndex == UINT32_MAX ) {
                    graphicsQueueNodeIndex = i;
                }

                if (supportsPresent[i] == VK_TRUE) {
                    graphicsQueueNodeIndex = i;
                    presentQueueNodeIndex = i;
                    break;
                }
            }
        }
        if ( presentQueueNodeIndex == UINT32_MAX ) 
        {
            // If didn't find a queue that supports both graphics and present, then
            // find a separate present queue.
            for ( uint32_t i = 0; i < vulInfo.queue_count; ++i ) 
            {
                if (supportsPresent[i] == VK_TRUE) 
                {
                    presentQueueNodeIndex = i;
                    break;
                }
            }
        }
        free(supportsPresent);

        // Generate error if could not find both a graphics and a present queue
        if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX) 
        {
            LOG.error("Swapchain Initialization Failure: Could not find a graphics and a present queue");
            return true;
        }

        // TODO: Add support for separate queues, including presentation,
        //       synchronization, and appropriate tracking for QueueSubmit.
        // NOTE: While it is possible for an application to use a separate graphics
        //       and a present queues, this demo program assumes it is only using
        //       one:
        if (graphicsQueueNodeIndex != presentQueueNodeIndex) {
            LOG.error("Swapchain Initialization Failure: Could not find a common graphics and a present queue");
            return true;
        }

        vulInfo.graphics_queue_node_index = graphicsQueueNodeIndex;

        vkGetDeviceQueue( vulInfo.device, vulInfo.graphics_queue_node_index, 0,
                     &vulInfo.queue);
                     
        // Get the list of VkFormat's that are supported:
        uint32_t formatCount;
        err = vulInfo.fpGetPhysicalDeviceSurfaceFormatsKHR(vulInfo.gpu, vulInfo.surfaces[p_window._GLFW_WindowHandle()], &formatCount, nullptr);
        assert(!err);

        VkSurfaceFormatKHR *surfFormats = (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
        err = vulInfo.fpGetPhysicalDeviceSurfaceFormatsKHR(vulInfo.gpu, vulInfo.surfaces[p_window._GLFW_WindowHandle()], &formatCount, surfFormats);
        assert(!err);

        // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
        // the surface has no preferred format.  Otherwise, at least one
        // supported format will be returned.
        if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
            vulInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
        } else {
            assert(formatCount >= 1);
            vulInfo.format = surfFormats[0].format;
        }

        vulInfo.color_space = surfFormats[0].colorSpace;


        // Get Memory information and properties
        vkGetPhysicalDeviceMemoryProperties(vulInfo.gpu, &vulInfo.memory_properties);


        // create command poll
        VkCommandPoolCreateInfo cmd_pool_info;
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = nullptr;
        cmd_pool_info.queueFamilyIndex = vulInfo.graphics_queue_node_index;
        cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        err = vkCreateCommandPool(vulInfo.device, &cmd_pool_info, nullptr, &vulInfo.cmd_pool);
        assert(!err);

        VkCommandBufferAllocateInfo cmd;
        cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd.pNext = nullptr;
        cmd.commandPool = vulInfo.cmd_pool;
        cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd.commandBufferCount = 1;

        err = vkAllocateCommandBuffers(vulInfo.device, &cmd, &vulInfo.draw_cmd);
        assert(!err);

        // prepare buffers

        VkSwapchainKHR oldSwapchain = vulInfo.swapchain;

        // Check the surface capabilities and formats
        VkSurfaceCapabilitiesKHR surfCapabilities;
        err = vulInfo.fpGetPhysicalDeviceSurfaceCapabilitiesKHR( vulInfo.gpu, vulInfo.surfaces[p_window._GLFW_WindowHandle()], &surfCapabilities );
        assert(!err);

        uint32_t presentModeCount;
        err = vulInfo.fpGetPhysicalDeviceSurfacePresentModesKHR( vulInfo.gpu, vulInfo.surfaces[p_window._GLFW_WindowHandle()], &presentModeCount, nullptr );
        assert(!err);

        VkPresentModeKHR *presentModes = (VkPresentModeKHR *)malloc(presentModeCount * sizeof(VkPresentModeKHR));
        assert(presentModes);

        err = vulInfo.fpGetPhysicalDeviceSurfacePresentModesKHR( vulInfo.gpu, vulInfo.surfaces[p_window._GLFW_WindowHandle()], &presentModeCount, presentModes );
        assert(!err);

        VkExtent2D swapchainExtent;
        // width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
        if (surfCapabilities.currentExtent.width == 0xFFFFFFFF)
        {
            
        }

        return false;
    }
}