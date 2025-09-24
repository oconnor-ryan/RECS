#####################
# Test Exclude
#####################

set(TEST_EXCLUDE "test_exclude")

add_executable(${TEST_EXCLUDE} 
  test_masks.c
)

# -Werror is very annoying, especially for testing
target_compile_options(${TEST_EXCLUDE} PRIVATE -fcolor-diagnostics -fansi-escape-codes -g -std=c11 -Wall -Wextra -pedantic  -Wundef)

target_include_directories(${TEST_EXCLUDE} PUBLIC 
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${TEST_EXCLUDE} ${ECS})

add_test(NAME ${TEST_EXCLUDE} COMMAND ${TEST_EXCLUDE})
