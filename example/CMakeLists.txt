#####################
# Example Reame
#####################

set(EXAMPLE_README "example_readme")

add_executable(${EXAMPLE_README} 
  ex_readme.c
)


target_compile_options(${EXAMPLE_README} PRIVATE 
  $<$<CXX_COMPILER_ID:Clang>:-fcolor-diagnostics -fansi-escape-codes> 
  -g 
  -std=c99
  -Wall 
  -Wextra 
  -pedantic  
  -Wundef
)

target_include_directories(${EXAMPLE_README} PUBLIC 
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${EXAMPLE_README} ${ECS})


#####################
# Example 1
#####################

set(EXAMPLE1 "example1")

add_executable(${EXAMPLE1} 
  ex1.c
)

target_compile_options(${EXAMPLE1} PRIVATE 
  $<$<CXX_COMPILER_ID:Clang>:-fcolor-diagnostics -fansi-escape-codes> 
  -g 
  -std=c99
  -Wall 
  -Wextra 
  -pedantic  
  -Wundef
)

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

target_compile_options(${EXAMPLE2} PRIVATE 
  $<$<CXX_COMPILER_ID:Clang>:-fcolor-diagnostics -fansi-escape-codes> 
  -g 
  -std=c99
  -Wall 
  -Wextra 
  -pedantic  
  -Wundef
)

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

target_compile_options(${EXAMPLE3} PRIVATE 
  $<$<CXX_COMPILER_ID:Clang>:-fcolor-diagnostics -fansi-escape-codes> 
  -g 
  -std=c99
  -Wall 
  -Wextra 
  -pedantic  
  -Wundef
)

target_include_directories(${EXAMPLE3} PUBLIC 
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${EXAMPLE3} ${ECS})
