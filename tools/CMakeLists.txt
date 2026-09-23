


set(BENCHMARK "benchmark")
add_executable(${BENCHMARK} 
  benchmark.c 
)

target_compile_options(${BENCHMARK} PRIVATE 
  $<$<CXX_COMPILER_ID:Clang>:-fcolor-diagnostics -fansi-escape-codes> 
  -g 
  -std=c99 
  -fexcess-precision=standard
  -Wall 
  -Wextra 
  -Wpedantic 
  -Wundef 
  -Werror 
  -Wno-error=unused-parameter 
  -Wno-unused-parameter

)
target_link_libraries(${BENCHMARK} recs $<$<CXX_COMPILER_ID:GNU>:m>)