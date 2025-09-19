cmake_minimum_required(VERSION 3.15)
project(recs VERSION 0.1 LANGUAGES C)

set(CMAKE_BUILD_TYPE "Debug")


set(CMAKE_C_STANDARD 11) # Requires C11 standard

enable_testing()

set(ECS "recs")


#add_subdirectory(example)


add_library(${ECS} STATIC 
  src/ecs/component_manager.c
  src/ecs/ecs.c
  src/ecs/entity_manager.c
  src/ecs/bitmask_handler.c
)

#-Werror was removed
target_compile_options(${ECS} PUBLIC -fcolor-diagnostics -fansi-escape-codes -g -std=c11 -Wall -Wextra -pedantic  -Wundef)


target_include_directories(${ECS} PUBLIC 
  src/
)

add_subdirectory(example)








