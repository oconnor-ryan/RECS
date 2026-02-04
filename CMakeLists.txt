cmake_minimum_required(VERSION 3.15)
project(recs VERSION 0.1 LANGUAGES C)

set(CMAKE_BUILD_TYPE "Debug")


set(CMAKE_C_STANDARD 99) # Requires C99 standard

enable_testing()

set(ECS "recs")

add_library(${ECS} STATIC 
  src/ecs.c
  src/entity_manager.c
  src/component_pool.c
  src/bitmask.c
)

#-Werror was removed
#added generator expressions so that we can include Clang specific compile options
target_compile_options(${ECS} PRIVATE $<$<CXX_COMPILER_ID:Clang>:-fcolor-diagnostics -fansi-escape-codes> -g -std=c99 -Wall -Wextra -pedantic  -Wundef)


target_include_directories(${ECS} PRIVATE 
  src/
)

target_include_directories(${ECS} PUBLIC "include/")

add_subdirectory(example)
add_subdirectory(tests)









