
set(EXAMPLE "example")

add_executable(${EXAMPLE} 
  test1.c
)

# -Werror is very annoying, especially for testing
target_compile_options(${EXAMPLE} PRIVATE -fcolor-diagnostics -fansi-escape-codes -g -std=c11 -Wall -Wextra -pedantic  -Wundef)

target_include_directories(${EXAMPLE} PUBLIC 
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${EXAMPLE} ${ECS})