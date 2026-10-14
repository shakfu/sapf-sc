# scsynth in the sapf executable, as nanosynth embeds it: static libscsynth with SC's own audio
# driver (CoreAudio on macOS, its bundled PortAudio elsewhere), libsndfile, and SC's core UGens as
# plugin modules in ${CMAKE_BINARY_DIR}/plugins.
#
# Both sources are pinned release tarballs, fetched once into the build directory, and SC's is
# patched by patch-supercollider.sh to build as a subdirectory. To build offline, point CMake at
# unpacked copies, running that script in SC's first:
#   -DFETCHCONTENT_SOURCE_DIR_SUPERCOLLIDER=/path/to/SuperCollider-3.14.1-Source
#   -DFETCHCONTENT_SOURCE_DIR_SNDFILE=/path/to/libsndfile-1.2.2
# Linux needs ALSA's headers for PortAudio (libasound2-dev or alsa-lib-devel).

include(FetchContent)

# Static libsndfile goes into DiskIO_UGens, a shared module, so everything here is built as
# position-independent code; Linux refuses to link it otherwise (macOS builds PIC anyway).
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

FetchContent_Declare(sndfile
	URL https://github.com/libsndfile/libsndfile/releases/download/1.2.2/libsndfile-1.2.2.tar.xz
	URL_HASH SHA256=3799ca9924d3125038880367bf1468e53a1b7e3686a934f098b7e1d286cdb80e
	EXCLUDE_FROM_ALL)
# The script's hash, an argument it ignores, puts its contents into the command, so editing it
# re-runs the patch on an existing build tree; FetchContent compares only the command string.
set(SAPF_SC_PATCH ${CMAKE_CURRENT_LIST_DIR}/patch-supercollider.sh)
file(SHA256 ${SAPF_SC_PATCH} SAPF_SC_PATCH_HASH)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SAPF_SC_PATCH})
FetchContent_Declare(supercollider
	URL https://github.com/supercollider/supercollider/releases/download/Version-3.14.1/SuperCollider-3.14.1-Source.tar.bz2
	URL_HASH SHA256=ee640c68777ae697682066ce5c4a8b7e56c5b223e76c79c13b5be5387ee55bb2
	PATCH_COMMAND sh ${SAPF_SC_PATCH} ${SAPF_SC_PATCH_HASH}
	EXCLUDE_FROM_ALL)

# libsndfile: static, no external codecs (no Ogg, FLAC or MPEG)
set(BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_EXTERNAL_LIBS OFF CACHE BOOL "" FORCE)
set(ENABLE_MPEG OFF CACHE BOOL "" FORCE)
set(ENABLE_CPACK OFF CACHE BOOL "" FORCE)
set(INSTALL_MANPAGES OFF CACHE BOOL "" FORCE)
set(INSTALL_PKGCONFIG_MODULE OFF CACHE BOOL "" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)  # libsndfile 1.2.2 asks for an older CMake
FetchContent_MakeAvailable(sndfile)

# SC finds libsndfile through these; the target name resolves at build time.
set(SNDFILE_LIBRARY sndfile CACHE STRING "" FORCE)
set(SNDFILE_INCLUDE_DIR "${sndfile_SOURCE_DIR}/include" CACHE PATH "" FORCE)

# SuperCollider: static libscsynth only
set(SC_QT OFF CACHE BOOL "" FORCE)
set(SC_IDE OFF CACHE BOOL "" FORCE)
set(SUPERNOVA OFF CACHE BOOL "" FORCE)
set(NO_X11 ON CACHE BOOL "" FORCE)
set(ENABLE_TESTSUITE OFF CACHE BOOL "" FORCE)
set(SC_HIDAPI OFF CACHE BOOL "" FORCE)
set(SC_ABLETON_LINK OFF CACHE BOOL "" FORCE)
set(INSTALL_HELP OFF CACHE BOOL "" FORCE)
set(SC_BUILD_SCLANG OFF CACHE BOOL "" FORCE)
set(SC_BUILD_EDITORS OFF CACHE BOOL "" FORCE)
set(LIBSCSYNTH OFF CACHE BOOL "" FORCE)        # static
set(SYSTEM_YAMLCPP ON CACHE BOOL "" FORCE)     # yaml-cpp is sclang's
set(FFT_GREEN ON CACHE BOOL "" FORCE)          # no FFTW on Linux
set(scappauxresourcesdir "" CACHE STRING "" FORCE)  # no SuperCollider.app bundle on macOS
set(scappbindir "" CACHE STRING "" FORCE)
if(NOT APPLE)
	set(AUDIOAPI portaudio CACHE STRING "" FORCE)
	set(SYSTEM_PORTAUDIO OFF CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(supercollider)

set(SAPF_SC_PLUGINS
	BinaryOpUGens ChaosUGens DelayUGens DemandUGens DynNoiseUGens FilterUGens GendynUGens GrainUGens
	IOUGens LFUGens MulAddUGens NoiseUGens OscUGens PanUGens PhysicalModelingUGens ReverbUGens TestUGens
	TriggerUGens UnaryOpUGens UnpackFFTUGens FFT_UGens PV_ThirdParty ML_UGens DiskIO_UGens)

add_library(sapf_server STATIC ${SAPF_ROOT}/src/server/EmbeddedServer.cpp)
target_include_directories(sapf_server PRIVATE
	${sndfile_SOURCE_DIR}/include
	${sndfile_BINARY_DIR}/include
	${supercollider_SOURCE_DIR}/include/server
	${supercollider_SOURCE_DIR}/include/common
	${supercollider_SOURCE_DIR}/include/plugin_interface
	${supercollider_SOURCE_DIR}/common)
target_link_libraries(sapf_server PUBLIC sapf_engine PRIVATE libscsynth tlsf sndfile)
target_compile_definitions(sapf_server PUBLIC SAPF_EMBED_SCSYNTH)
if(APPLE)
	target_link_libraries(sapf_server PRIVATE "-framework CoreAudio" "-framework Accelerate" "-framework CoreServices"
		"-framework Foundation" "-framework AppKit")
elseif(UNIX)
	target_link_libraries(sapf_server PRIVATE pthread rt)
endif()
add_dependencies(sapf_server ${SAPF_SC_PLUGINS})

# the plugins beside the executable, where the server looks for them
add_custom_command(TARGET sapf_server POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/plugins")
foreach(p ${SAPF_SC_PLUGINS})
	add_custom_command(TARGET sapf_server POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${p}>" "${CMAKE_BINARY_DIR}/plugins/")
endforeach()
