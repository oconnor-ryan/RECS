

#####################
# Example 1
#####################

set(EXAMPLE1 "example1")

add_executable(${EXAMPLE1} 
  ex1.c
)

# -Werror is very annoying, especially for testing
target_compile_options(${EXAMPLE1} PRIVATE -fcolor-diagnostics -fansi-escape-codes -g -std=c11 -Wall -Wextra -pedantic  -Wundef)

target_include_directories(${EXAMPLE1} PUBLIC 
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${EXAMPLE1} ${ECS})



#####################
# Example 2
#####################


set(EXAMPLE2 "example2")

add_executable(${EXAMPLE2} 
  ex2.c
)

# -Werror is very annoying, especially for testing
target_compile_options(${EXAMPLE2} PRIVATE -fcolor-diagnostics -fansi-escape-codes -g -std=c11 -Wall -Wextra -pedantic  -Wundef)

target_include_directories(${EXAMPLE2} PUBLIC 
  ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(${EXAMPLE2} ${ECS})


#####################
# Example 3
#####################


set(EXAMPLE3 "example3")

add_executable(${EXAMPLE3} 
  ex3.c
)

# -Werror is very annoying, especially for testing
target_compile_options(${EXAMPLE3} PRIVATE -fcolor-diagnostics -fansi-escape-codes -g -std=c11 -Wall -Wextra -pedantic  -Wundef)

target_include_directories(${EXAMPLE3} PUBLIC 
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${EXAMPLE3} ${ECS})
