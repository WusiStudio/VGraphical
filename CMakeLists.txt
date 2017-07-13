cmake_minimum_required(VERSION 3.9.0)

project(VGraphical)

option(BUILD_BY_OPENGL "build by opengl api" OFF)
option(BUILD_BY_VULKAN "build by vulkan api" ON)

include_directories(include)
include_directories(${TOOLS_INCLUDE_DIR})
include_directories(${GLFW_INCLUDE_DIR})

aux_source_directory(src SRCS)

if(BUILD_BY_OPENGL)
    include_directories(src/opengl/include)
    aux_source_directory(src/opengl/src SRCS)
endif()

if(BUILD_BY_VULKAN)
    include_directories(src/vulkan/include)
    aux_source_directory(src/vulkan/src SRCS)
    
    if(VULKAN_INCLUDE_DIR)
        include_directories(${VULKAN_INCLUDE_DIR})
    endif()
endif()

add_library(${PROJECT_NAME} ${SRCS})

#设置编译选项-------------------------------------------
IF(WIN32)
    # DEBUG RELEASE
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -std=c++11 -Wall")
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -std=c++11 -Wall")

ELSEIF(APPLE)
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -m64 -std=c++11 -O0 -Wall -g")
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -m64 -std=c++11 -O3 -Wall")

ELSEIF(UNIX)
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -m64 -std=c++11 -O0 -Wall -g")
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -m64 -std=c++11 -O3 -Wall")
ENDIF()