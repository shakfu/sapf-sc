cmake_minimum_required(VERSION 3.15)

project(sapf_sc)

# Store the sapf root directory for use in subdirectories
set(SAPF_ROOT ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "SAPF root directory")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Optional: Disables compiler extensions beyond the standard

set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)

option(UNIVERSAL "Build Universal Binaries" OFF)

# use ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Found ccache in ${CCACHE_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()


message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if (NOT CMAKE_OSX_ARCHITECTURES)
        if(UNIVERSAL)
            set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architecture" FORCE)
        else()
            set(CMAKE_OSX_ARCHITECTURES ${CMAKE_SYSTEM_PROCESSOR} CACHE STRING "macOS architecture" FORCE)
        endif()
        message("CMAKE_OSX_ARCHITECTURES set to ${CMAKE_OSX_ARCHITECTURES}")
    endif()
endif()


if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")


set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)

# Replxx for cross-platform line editing
set(REPLXX_BUILD_EXAMPLES OFF CACHE BOOL "Disable replxx examples" FORCE)
add_subdirectory(thirdparty/replxx)

add_subdirectory(src/engine)

# scsynth in the executable, so play needs no server; off for builds that only write defs
option(SAPF_EMBED_SCSYNTH "Embed scsynth in the sapf executable" ON)
if(SAPF_EMBED_SCSYNTH)
	include(cmake/EmbeddedScsynth.cmake)
endif()

add_subdirectory(src/cli)

# Testing
enable_testing()

# Google Test for unit tests (vendored in thirdparty/)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(thirdparty/googletest)

add_subdirectory(tests)
