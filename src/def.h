#ifndef ECS_DEF_H
#define ECS_DEF_H


#ifndef MALLOC
  #include <stdlib.h>
  #define MALLOC(size) malloc(size)
#endif

#ifndef FREE
  #include <stdlib.h>
  #define FREE(ptr) free(ptr)
#endif

#ifndef ASSERT
  #include <assert.h>
  #define ASSERT(boolean) assert(boolean)
#endif

#define NO_ENTITY_ID 0xFFFFFFFF

#endif// ECS_DEF_H

