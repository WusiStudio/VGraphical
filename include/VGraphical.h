#pragma once
#ifndef __V_GRAPHICAL_H__
#define __V_GRAPHICAL_H__

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#ifndef ROOT_SPACE
#define ROOT_SPACE ws
#endif //ROOT_SPACE

#include "window.h"

namespace ROOT_SPACE
{
    class VGraphical
    {
    public:
        static bool initGraphical(void);
        static bool initWindow( window & p_window );
        static void __glfw_error_callback( int p_error, const char * p_description );
    };
}

#endif //__V_GRAPHICAL_H__