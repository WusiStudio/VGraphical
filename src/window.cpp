#include "window.h"
#include "log.hpp"
#include "VGraphical.h"

namespace ROOT_SPACE
{

    std::map< GLFWwindow * , window * > window::smWindows;


    void window::setWindowSize( const glm::ivec2 & p_windowSize )
    {
        glfwSetWindowSize( mWindowHandle, p_windowSize.x, p_windowSize.y );
    }
    void window::setWindowPos( const glm::ivec2 & p_windowPos )
    {
        glfwSetWindowSize( mWindowHandle, p_windowPos.x, p_windowPos.y );
    }

    void window::setWindowTitle( const std::string & p_windowTitle )
    {
        mWindowTitle = p_windowTitle;
        glfwSetWindowTitle( mWindowHandle, mWindowTitle.c_str() );
    }

    GLFWwindow * window::_GLFW_WindowHandle(void) const
    {
        return mWindowHandle;
    }

    window::window( void )
    {
        mWindowHandle = nullptr;
        mWindowSize = glm::ivec2( 600, 500 );
        mWindowPos = glm::ivec2( 0, 0 );
        mWindowTitle = "Humble";
    }

    window::~window( void )
    {
        if( mWindowHandle )
        {
            smWindows.erase( mWindowHandle );
            glfwDestroyWindow( mWindowHandle );
            mWindowHandle = nullptr;
        }
    }

    void window::__refresh_callback( GLFWwindow * p_window )
    {
        if( smWindows.find( p_window ) != smWindows.end() )
        {
            smWindows[p_window]->onRefresh();
        }
    }

    void window::__key_callback( GLFWwindow * p_window, int p_key, int p_scancode, int p_action, int p_mods )
    {
        if( smWindows.find( p_window ) != smWindows.end() )
        {
            smWindows[p_window]->onKeyCallBack( p_key, p_scancode, p_action, p_mods );
        }
    }

    void window::__resize_callback( GLFWwindow* p_window, int p_width, int p_height )
    {
        if( smWindows.find( p_window ) != smWindows.end() )
        {
            smWindows[p_window]->onResize( glm::ivec2( p_width, p_height ) );
        }
    }

    void window::__pos_callback( GLFWwindow* p_window, int p_x, int p_y )
    {
        if( smWindows.find( p_window ) != smWindows.end() )
        {
            smWindows[p_window]->onPosChanged( glm::ivec2( p_x, p_y ) );
        }
    }

    bool window::init( void )
    {
        if( object::init() )
        {
            return true;
        }

        glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

        mWindowHandle = glfwCreateWindow( mWindowSize.x, mWindowSize.y, mWindowTitle.c_str(), nullptr, nullptr );
        if( !mWindowHandle )
        {
            LOG.error("Cannot create a window in which to draw!");
            return true;
        }
        
        smWindows[mWindowHandle] = this;

        // glfwSetWindowUserPointer( mWindowHandle, this );
        glfwSetKeyCallback( mWindowHandle, window::__key_callback );
        glfwSetWindowRefreshCallback( mWindowHandle, window::__refresh_callback );
        glfwSetFramebufferSizeCallback( mWindowHandle, window::__resize_callback );
        // glfwSetWindowPosCallback( mWindowHandle,  )

        if( VGraphical::initWindow( *this ) )
        {
            return true;
        }

        return false;
    }

    bool window::initWithInfo( const uint32_t p_width, const uint32_t p_height, const std::string & p_title )
    {
        mWindowSize = glm::ivec2( p_width, p_height );
        mWindowTitle = p_title;
        return init();
    }


	bool window::destory ( void )
	{
        
        if( mWindowHandle )
        {
            smWindows.erase( mWindowHandle );
            glfwDestroyWindow( mWindowHandle );
            mWindowHandle = nullptr;
        }
        
		return object::destory ();
	}

    void window::onRefresh(void)
    {
        
    }

    void window::onKeyCallBack( const int p_key, const int p_scancode, const int p_action, const int p_mods )
    {
        LOG.info( "p_key: {0}, p_scancode: {1}, p_action: {2}, p_mods: {3}", p_key, p_scancode, p_action, p_mods );
    }

    void window::onResize( const glm::ivec2 & p_windowSize )
    {
        mWindowSize = p_windowSize;
        mWindowSizeChanged( mWindowSize );
    }

    void window::onPosChanged( const glm::ivec2 & p_windowPos )
    {
        mWindowPos = p_windowPos;
        mWindowPosChanged( mWindowPos );
    }
}