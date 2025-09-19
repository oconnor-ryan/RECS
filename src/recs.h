#ifndef RECS_H
#define RECS_H

#include <stdint.h>
#include <stddef.h>

#ifndef RECS_MALLOC
  #include <stdlib.h>
  #define RECS_MALLOC(size) malloc(size)
#endif

#ifndef RECS_FREE
  #include <stdlib.h>
  #define RECS_FREE(ptr) free(ptr)
#endif

#ifndef RECS_ASSERT
  #include <assert.h>
  #define RECS_ASSERT(boolean) assert(boolean)
#endif

#define RECS_NO_ENTITY_ID 0xFFFFFFFF


typedef uint32_t recs_component;
typedef uint32_t recs_entity;
typedef uint32_t recs_tag;
typedef uint32_t recs_system_group;



typedef struct recs *recs;


typedef void (*recs_system_func)(struct recs *ecs);

//convienence macros for initializing enums for component ids, tags, and system groups.
//this reduces the chance of a user making the mistake of defining custom values in their enum definition,
//which would prevent the ECS from working correctly.
// This is due to each enum needing to be defined such that each enum value is defined as 0, 1, 2, 3, ...,
// since these are indices to arrays.

#define RECS_INIT_COMP_IDS(comp_enum_name, ...) enum comp_enum_name { __VA_ARGS__ }
#define RECS_INIT_TAG_IDS(tag_enum_name, ...) enum tag_enum_name { __VA_ARGS__ }
#define RECS_INIT_SYS_GRP_IDS(sys_enum_name, ...) enum sys_enum_name { __VA_ARGS__ }


/*
  You must define the RECS_COMP_TO_ID_MAPPER macro if using RECS_MAP_COMP_TO_ID or RECS_MAP_COMP_PTR_TO_ID.

  The RECS_COMP_TO_ID_MAPPER must be written in a format similar to the _Generic keyword:

  #define RECS_COMP_TO_ID_MAPPER \
    <type>: <ENUM_OR_INTEGER_VALUE>, \
    <type>: <ENUM_OR_INTEGER_VALUE>, \
    <type>: <ENUM_OR_INTEGER_VALUE>, \
    ... (not part of macro definition, it just means that you can add as many <type>: <value> pairs as needed)
    <type>: <ENUM_OR_INTEGER_VALUE> 

  Here is an example of a definition for RECS_COMP_TO_ID_MAPPER
*/
/* 
#define RECS_COMP_TO_ID_MAPPER \
  struct componentA: COMP_ID_A_ENUM,
  struct componentB: COMP_ID_B_ENUM
*/


//using the type of the passed-in variable, grab the component ID associated with the variable's type
#define RECS_MAP_COMP_TO_ID(comp_variable) _Generic((comp_variable), \
  RECS_COMP_TO_ID_MAPPER)


//similar to RECS_MAP_COMP_TO_ID, except the comp_variable is a pointer to the type whose component ID you want.
#define RECS_MAP_COMP_PTR_TO_ID(comp_variable) _Generic((*comp_variable), \
  RECS_COMP_TO_ID_MAPPER)


#define RECS_ENTITY_ADD_COMP(ecs, entity, comp_var) recs_entity_add_component(ecs, entity, RECS_MAP_COMP_TO_ID(comp_var), &comp_var)

recs recs_init(uint32_t max_entities, uint32_t max_components, uint32_t max_tags, uint32_t max_systems, uint32_t max_system_groups, void *context);
void recs_free(struct recs *recs);

int recs_component_register(struct recs *recs, recs_component type, uint32_t max_instances, size_t comp_size);
void* recs_component_get(struct recs *recs, recs_component c, uint32_t index);



void recs_system_register(struct recs *recs, recs_system_func func, recs_system_group type);

void recs_system_set_context(struct recs *recs, void *context);
void* recs_system_get_context(struct recs *recs);

void recs_system_run(struct recs *recs, recs_system_group type);


uint32_t recs_num_active_entities(struct recs *recs);
uint32_t recs_max_entities(struct recs *recs);



recs_entity recs_entity_add(struct recs *recs);
void recs_entity_remove(struct recs *recs, recs_entity e);
void recs_entity_remove_at_id_index(struct recs *recs, uint32_t id_index);

recs_entity recs_entity_get(struct recs *recs, uint32_t index);


void recs_entity_add_component(struct recs *recs, recs_entity e, recs_component comp_type, void *component);
void recs_entity_add_tag(struct recs *recs, recs_entity e, recs_tag tag);

void recs_entity_remove_component(struct recs *recs, recs_entity e, recs_component comp_type);
void recs_entity_remove_tag(struct recs *recs, recs_entity e, recs_tag tag);
void recs_entity_remove_all_components(struct recs *recs, recs_entity e);

int recs_entity_has_component(struct recs *recs, recs_entity e, recs_component c);
int recs_entity_has_tag(struct recs *recs, recs_entity e, recs_tag tag);

void* recs_entity_get_component(struct recs *recs, recs_entity e, recs_component c);




#endif// RECS_H
