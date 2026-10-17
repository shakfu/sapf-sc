# stac CLAP plugin: ImGui editor (pugl + OpenGL), loopback UDP input, scsynth driven by the host.
add_subdirectory(${SAPF_ROOT}/thirdparty/pugl ${CMAKE_BINARY_DIR}/thirdparty/pugl)
add_subdirectory(${SAPF_ROOT}/thirdparty/imgui ${CMAKE_BINARY_DIR}/thirdparty/imgui)

add_library(stac_clap MODULE stac_clap.cpp udp_server.cpp editor_imgui.cpp editor_logic.cpp)
target_link_libraries(stac_clap PRIVATE sapf_server pugl_plugin imgui_gl3)
target_include_directories(stac_clap PRIVATE ${SAPF_ROOT}/thirdparty/clap/include)
target_compile_features(stac_clap PRIVATE cxx_std_17)
set_target_properties(stac_clap PROPERTIES OUTPUT_NAME stac CXX_VISIBILITY_PRESET hidden)

if(APPLE)
	target_compile_definitions(stac_clap PRIVATE GL_SILENCE_DEPRECATION)
	target_link_options(stac_clap PRIVATE -exported_symbols_list ${SAPF_ROOT}/thirdparty/clap/src/macos-symbols.txt)
	set_target_properties(stac_clap PROPERTIES
		BUNDLE TRUE
		BUNDLE_EXTENSION clap
		MACOSX_BUNDLE_GUI_IDENTIFIER com.github.shakfu.stac
		MACOSX_BUNDLE_BUNDLE_NAME stac
		MACOSX_BUNDLE_BUNDLE_VERSION 0.1.22
		MACOSX_BUNDLE_SHORT_VERSION_STRING 0.1.22
		MACOSX_BUNDLE_INFO_PLIST ${SAPF_ROOT}/thirdparty/clap/src/plugins.plist.in)
	set(STAC_CLAP_RESOURCES "$<TARGET_BUNDLE_CONTENT_DIR:stac_clap>/Resources")
else()
	set_target_properties(stac_clap PROPERTIES PREFIX "" SUFFIX ".clap")
	# Exports clap_entry only: the static engine and libscsynth stay out of the host's symbol space.
	target_link_options(stac_clap PRIVATE -Wl,--exclude-libs,ALL)
	set(STAC_CLAP_RESOURCES "$<TARGET_FILE_DIR:stac_clap>/stac")
endif()

# The UGens and the bundled modules, where stac_clap.cpp's ResourceDir looks for them.
add_custom_command(TARGET stac_clap POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory "${STAC_CLAP_RESOURCES}/plugins"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SAPF_ROOT}/prelude.stac ${SAPF_ROOT}/std.stac ${SAPF_ROOT}/sapf.stac
		"${STAC_CLAP_RESOURCES}/")
foreach(p ${SAPF_SC_PLUGINS})
	add_custom_command(TARGET stac_clap POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${p}>" "${STAC_CLAP_RESOURCES}/plugins/")
endforeach()

# Interactive host: the plugin's editor in a window, audio to the default output (`make editor`).
add_executable(stac_dev_host dev_host.cpp)
target_link_libraries(stac_dev_host PRIVATE pugl_host rtaudio ${CMAKE_DL_LIBS})
target_include_directories(stac_dev_host PRIVATE ${SAPF_ROOT}/thirdparty/clap/include ${SAPF_ROOT}/thirdparty/rtaudio)
target_compile_features(stac_dev_host PRIVATE cxx_std_17)
target_compile_definitions(stac_dev_host PRIVATE STAC_CLAP_PLUGIN_PATH="$<TARGET_FILE:stac_clap>")
add_dependencies(stac_dev_host stac_clap)

# Headless host: loads the built plugin, drives it over UDP, renders audio.
add_executable(test_clap_host test_clap_host.cpp)
target_include_directories(test_clap_host PRIVATE ${SAPF_ROOT}/thirdparty/clap/include)
target_compile_features(test_clap_host PRIVATE cxx_std_17)
target_link_libraries(test_clap_host PRIVATE ${CMAKE_DL_LIBS})
add_dependencies(test_clap_host stac_clap)
add_test(NAME example_clap_plugin COMMAND test_clap_host $<TARGET_FILE:stac_clap>)
set_tests_properties(example_clap_plugin PROPERTIES ENVIRONMENT "STAC_CLAP_UDP_PORT=57199" TIMEOUT 60)

include(GoogleTest)
add_executable(test_editor_logic test_editor_logic.cpp editor_logic.cpp)
target_link_libraries(test_editor_logic PRIVATE GTest::gtest_main)
target_compile_features(test_editor_logic PRIVATE cxx_std_17)
gtest_discover_tests(test_editor_logic)
