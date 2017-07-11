#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "vulkanInfo.h"
#include "VGraphical.h"
#include "log.hpp"
#include <cassert>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

vulkanInfo vulkanInfo::instance;

namespace ROOT_SPACE
{

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
        VkResult err;

        vulkanInfo & vulInfo = vulkanInfo::instance;
		vulInfo.validate = true;

        vulInfo.enabled_layer_count = 0;
        vulInfo.enabled_extension_count = 0;

        const char **instance_validation_layers = NULL;


        char *instance_validation_layers_alt1[] = {
            "VK_LAYER_LUNARG_standard_validation"
        };

        char *instance_validation_layers_alt2[] = {
            "VK_LAYER_GOOGLE_threading",       "VK_LAYER_LUNARG_parameter_validation",
            "VK_LAYER_LUNARG_object_tracker",  "VK_LAYER_LUNARG_image",
            "VK_LAYER_LUNARG_core_validation", "VK_LAYER_LUNARG_swapchain",
            "VK_LAYER_GOOGLE_unique_objects"
        };


        if (!glfwVulkanSupported())
        {
            LOG.error("GLFW failed to find the Vulkan loader.\nExiting ...");
            return true;
        }
       
        /* Look for validation layers */
        VkBool32 validation_found = 0;
        if( vulInfo.validate )
        {
            //get instance layer count
            uint32_t instance_layer_count = 0;
            err = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
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
        err = vkEnumerateInstanceExtensionProperties( NULL, &instance_extension_count, NULL );
        assert( !err );

        if ( instance_extension_count > 0 ) 
        {
            VkExtensionProperties *instance_extensions = (VkExtensionProperties *)malloc( sizeof( VkExtensionProperties ) * instance_extension_count);
            err = vkEnumerateInstanceExtensionProperties( NULL, &instance_extension_count, instance_extensions);
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
		app.pNext = NULL;
		app.pApplicationName = "";
		app.applicationVersion = 0;
		app.pEngineName = "";
		app.engineVersion = 0;
		app.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo inst_info;
		inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		inst_info.pNext = NULL;
		inst_info.pApplicationInfo = &app;
		inst_info.enabledLayerCount = vulInfo.enabled_layer_count;
		inst_info.ppEnabledLayerNames = (const char *const *)instance_validation_layers;
		inst_info.enabledExtensionCount = vulInfo.enabled_extension_count;
		inst_info.ppEnabledExtensionNames = (const char *const *)vulInfo.extension_names;

        uint32_t gpu_count;

        err = vkCreateInstance( &inst_info, NULL, &vulInfo.inst );
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
        err = vkEnumeratePhysicalDevices( vulInfo.inst, &gpu_count, NULL );
        assert( !err && gpu_count > 0 );

        if ( gpu_count > 0 ) 
        {
            VkPhysicalDevice *physical_devices = (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * gpu_count);
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

        err = vkEnumerateDeviceExtensionProperties( vulInfo.gpu, NULL, &device_extension_count, NULL );

        assert( !err );

        if (device_extension_count > 0) 
        {
            VkExtensionProperties *device_extensions = (VkExtensionProperties *)malloc (sizeof( VkExtensionProperties ) * device_extension_count );
            err = vkEnumerateDeviceExtensionProperties( vulInfo.gpu, NULL, &device_extension_count, device_extensions );
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
            dbgCreateInfo.pNext = NULL;

            err = vulInfo.CreateDebugReportCallback(vulInfo.inst, &dbgCreateInfo, NULL,
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

        LOG.info("hello vulkan demo");

        return false;
    }
}