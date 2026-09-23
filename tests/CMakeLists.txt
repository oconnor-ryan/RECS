#####################
# Test Exclude
#####################

set(TEST_EXCLUDE "test_exclude")

add_executable(${TEST_EXCLUDE} 
  test_masks.c
)


#-Werror was removed
#added generator expressions so that we can include Clang specific compile options
target_compile_options(${TEST_EXCLUDE} PRIVATE 
  $<$<CXX_COMPILER_ID:Clang>:-fcolor-diagnostics -fansi-escape-codes> 
  -g 
  -std=c11
  -Wall 
  -Wextra 
  -pedantic  
  -Wundef
)

target_include_directories(${TEST_EXCLUDE} PUBLIC 
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${TEST_EXCLUDE} ${ECS})

add_test(NAME ${TEST_EXCLUDE} COMMAND ${TEST_EXCLUDE})



#####################
# Build All Tests
#####################

set(BUILD_TESTS "build_tests")
add_custom_target(${BUILD_TESTS})
add_dependencies(${BUILD_TESTS} ${TEST_EXCLUDE})
