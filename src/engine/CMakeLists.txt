set(ENGINE_SOURCES
	CoreOps.cpp
	elapsedTime.cpp
	ErrorCodes.cpp
	MathFuns.cpp
	MathOps.cpp
	MultichannelExpansion.cpp
	Object.cpp
	Opcode.cpp
	Parser.cpp
	primes.cpp
	RandomOps.cpp
	RCObj.cpp
	SetOps.cpp
	SapfEngine.cpp
	StreamOps.cpp
	symbol.cpp
	Types.cpp
	Graph.cpp
	SCUGens.cpp
	SpecUGens.cpp
	EnvOps.cpp
	DefOps.cpp
	ResourceOps.cpp
	PatternOps.cpp
	Modules.cpp
	Completion.cpp
	Scsynth.cpp
	Texture.cpp
	Wavetables.cpp
	Samples.cpp
	VM.cpp
)

if(APPLE)
	list(APPEND ENGINE_SOURCES platform/PlatformMac.cpp)
elseif(WIN32)
	list(APPEND ENGINE_SOURCES platform/PlatformWindows.cpp)
else()
	list(APPEND ENGINE_SOURCES platform/PlatformUnix.cpp)
endif()

add_library(sapf_engine STATIC ${ENGINE_SOURCES})

target_include_directories(
	sapf_engine
	PUBLIC
	${SAPF_ROOT}/include
)

if(MSVC)
	target_compile_options(sapf_engine PUBLIC /W3)
else()
	target_compile_options(
		sapf_engine
		PUBLIC
		-Wmissing-field-initializers
		-Wreturn-type
		-Wparentheses
		-Wshadow
		-Wsign-compare
		-Wuninitialized
		-Wno-unused-function
		-Wunused-label
		-Wunused-value
		-Wunused-variable
	)
endif()

# highlights.scm, compiled in, so the REPL and editors read one query
if(SAPF_TREESITTER)
	set(STAC_HIGHLIGHTS ${SAPF_ROOT}/tree-sitter-stac/queries/highlights.scm)
	file(READ ${STAC_HIGHLIGHTS} STAC_HIGHLIGHTS_SCM)
	configure_file(${SAPF_ROOT}/cmake/StacHighlights.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/StacHighlights.h @ONLY)
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${STAC_HIGHLIGHTS})
	target_sources(sapf_engine PRIVATE Highlight.cpp)
	target_include_directories(sapf_engine PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
	target_compile_definitions(sapf_engine PUBLIC SAPF_TREESITTER=1)
	target_link_libraries(sapf_engine PUBLIC tree-sitter-stac)
endif()

if(SAPF_REPLXX)
	target_include_directories(sapf_engine PUBLIC ${SAPF_ROOT}/thirdparty/replxx/include)
	target_link_libraries(sapf_engine PUBLIC replxx)
else()
	target_compile_definitions(sapf_engine PUBLIC USE_REPLXX=0)
endif()

if(APPLE)
	target_link_libraries(
		sapf_engine
		PUBLIC
		"-framework Accelerate"
		"-framework CoreFoundation"
	)
elseif(UNIX)
	target_link_libraries(sapf_engine PUBLIC pthread m)
endif()

set_target_properties(
	sapf_engine
	PROPERTIES
	OUTPUT_NAME "sapf"
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
