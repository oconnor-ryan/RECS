cmake_minimum_required(VERSION 3.15)
project(ecs)

set(CMAKE_BUILD_TYPE "Debug")


set(CMAKE_C_STANDARD 11) # Requires C11 standard

enable_testing()

set(ECS "ecs")


#add_subdirectory(example)


add_library(${ECS} STATIC 
  src/component_manager.c
  src/ecs.c
  src/entity_manager.c
  src/util/stack.c
  src/bitmask_handler.c
)

#-Werror was removed
target_compile_options(${ECS} PUBLIC -fcolor-diagnostics -fansi-escape-codes -g -std=c99 -Wall -Wextra -pedantic  -Wundef)


target_include_directories(${ECS} PUBLIC 
  src/
)

add_subdirectory(example)








