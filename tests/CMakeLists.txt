# SAPF Tests using CTest

# Unit tests (C++ with Google Test)
add_subdirectory(unit)

# Integration tests (sapf language)

# Smoke test - minimal test to verify sapf starts
add_test(
    NAME smoke
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Split unit tests for better failure isolation
add_test(
    NAME test_equals
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_equals.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_tuples
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_tuples.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_stack
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_stack.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_size_reverse
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_size_reverse.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_conditionals
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_conditionals.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_math
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_math.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_arrays
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_arrays.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_streams
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_streams.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_ordering
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_ordering.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_at
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_at.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_filter
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_filter.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_deep_mapping
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_deep_mapping.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_forms
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_forms.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME test_inheritance
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_CURRENT_SOURCE_DIR}/test_inheritance.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# sapf's language test suite; prints Passed when every case holds
add_test(
    NAME unit_tests_txt
    COMMAND $<TARGET_FILE:sapf> -q ${CMAKE_SOURCE_DIR}/unit-tests.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(unit_tests_txt PROPERTIES PASS_REGULAR_EXPRESSION "Passed" ENVIRONMENT "SAPF_PRELUDE=/dev/null")
