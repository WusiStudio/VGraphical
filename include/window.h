#pragma once
#ifndef __WINDOW_H__
#define __WINDOW_H__

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <string>
#include <map>
#include <functional>

#include "IMemory.h"
#include "glm.hpp"

namespace ROOT_SPACE
{
    class window: public object
    {
        ATTIRBUTE_R( glm::ivec2, WindowSize );
        ATTIRBUTE_R( glm::ivec2, WindowPos );
        ATTIRBUTE_R( std::string, WindowTitle );

        EVENT( std::function< void( const glm::ivec2 & ) >, WindowSizeChanged );
        EVENT( std::function< void( const glm::ivec2 & ) >, WindowPosChanged );
        
    public:
		CREATEFUNC ( window );

        void setWindowSize( const glm::ivec2 & p_windowSize );
        void setWindowPos( const glm::ivec2 & p_windowPos );
        void setWindowTitle( const std::string & p_windowTitle );

        GLFWwindow * _GLFW_WindowHandle(void) const;
        
    protected:
        window(void);
        ~window(void);

        static void __key_callback( GLFWwindow* p_window, int p_key, int p_scancode, int p_action, int p_mods );
        static void __refresh_callback( GLFWwindow* p_window );
        static void __resize_callback( GLFWwindow* p_window, int p_width, int p_height );
        static void __pos_callback( GLFWwindow* p_window, int p_x, int p_y );

        virtual bool init(void) override;
        virtual bool initWithInfo( const uint32_t p_width, const uint32_t p_height, const std::string & p_title );
        virtual bool destory(void) override;

        virtual void onRefresh(void);
        virtual void onKeyCallBack( const int p_key, const int p_scancode, const int p_action, const int p_mods );
        virtual void onResize( const glm::ivec2 & p_windowSize  );
        virtual void onPosChanged( const glm::ivec2 & p_windowPos );
        
    private:

        static std::map< GLFWwindow * , window * > smWindows;

        GLFWwindow *    mWindowHandle;

        // std::function< void ( const int p_key ) > mKeyDown;
        // std::function< void ( const int p_key ) > mKeyUp;
        
    };
};

#endif //__WINDOW_H__