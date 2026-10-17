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

# The CLAP plugin, examples/clap; needs the embedded scsynth. Its static libraries go into a
# shared module, so everything is position-independent.
option(STAC_BUILD_CLAP "Build the stac CLAP plugin" ON)
if(STAC_BUILD_CLAP)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Replxx for the REPL's line editing; off for an embedding host, such as the SSP plugin
option(SAPF_REPLXX "Line editing with replxx" ON)
if(SAPF_REPLXX)
	set(REPLXX_BUILD_EXAMPLES OFF CACHE BOOL "Disable replxx examples" FORCE)
	add_subdirectory(thirdparty/replxx)
endif()

# Syntax highlighting in the REPL with tree-sitter and tree-sitter-stac/, whose generated parser.c is
# committed, so the build needs no tree-sitter CLI
option(SAPF_TREESITTER "Syntax highlighting with tree-sitter" ON)
if(SAPF_TREESITTER)
	add_library(tree-sitter STATIC thirdparty/tree-sitter/lib/src/lib.c)
	target_include_directories(tree-sitter PUBLIC thirdparty/tree-sitter/lib/include PRIVATE thirdparty/tree-sitter/lib/src)
	target_compile_definitions(tree-sitter PRIVATE _POSIX_C_SOURCE=200112L _DEFAULT_SOURCE)
	set_target_properties(tree-sitter PROPERTIES C_STANDARD 11)
	add_library(tree-sitter-stac STATIC tree-sitter-stac/src/parser.c tree-sitter-stac/src/scanner.c)
	target_include_directories(tree-sitter-stac PRIVATE tree-sitter-stac/src)
	target_link_libraries(tree-sitter-stac PUBLIC tree-sitter)
	set_target_properties(tree-sitter-stac PROPERTIES C_STANDARD 11)
	if(NOT MSVC)
		target_compile_options(tree-sitter PRIVATE -w)
		target_compile_options(tree-sitter-stac PRIVATE -w)
	endif()
endif()

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

if(STAC_BUILD_CLAP AND SAPF_EMBED_SCSYNTH)
	set(RTAUDIO_BUILD_TESTING OFF CACHE BOOL "" FORCE)  # rtaudio is the dev host's audio output
	set(RTAUDIO_TARGETNAME_UNINSTALL "rtaudio_uninstall" CACHE STRING "" FORCE)
	add_subdirectory(thirdparty/rtaudio)
	add_subdirectory(examples/clap)
endif()
