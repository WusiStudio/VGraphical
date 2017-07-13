#include "VGraphical.h"

#include "log.hpp"

namespace ROOT_SPACE
{
    void VGraphical::__glfw_error_callback( int p_error, const char * p_description )
    {
        LOG.error( "glfw error[{0}]: {1}", p_error, p_description );
    }
}