add_executable(
	stac
	main.cpp
	ReplRunner.cpp
)

target_include_directories(
	stac
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
	stac
	PRIVATE
	$<IF:$<TARGET_EXISTS:sapf_server>,sapf_server,sapf_engine>
)

set_target_properties(
	stac
	PROPERTIES 
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
