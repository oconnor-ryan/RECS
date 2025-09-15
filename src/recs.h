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

#define NO_ENTITY_ID 0xFFFFFFFF


typedef uint32_t component_type;
typedef uint32_t entity;
typedef uint32_t tag_id;


//because game updates/physics may run on a separate time-step than
//game renders/draws, we will allow the option to specify which set
//of systems should run.

enum recs_system_type {
  RECS_SYSTEM_TYPE_UPDATE,
  RECS_SYSTEM_TYPE_RENDER,

};

typedef struct recs *recs;


typedef void (*recs_system_func)(struct recs *ecs, void* context);


recs recs_init(uint32_t max_entities, uint32_t max_component_types, uint32_t max_tags, uint32_t max_systems);
void recs_free(struct recs *recs);

int recs_component_register(struct recs *recs, component_type type, uint32_t max_components, size_t comp_size);
void* recs_component_get(struct recs *recs, component_type c, uint32_t index);



void recs_system_register(struct recs *recs, recs_system_func func, void *context, enum recs_system_type type);

void recs_system_set_context(struct recs *recs, uint32_t system_index, void *context);
void recs_system_run(struct recs *recs, uint32_t system_index);
void recs_system_run_all_with_type(struct recs *recs, enum recs_system_type type);


uint32_t recs_num_active_entities(struct recs *recs);
uint32_t recs_max_entities(struct recs *recs);



entity recs_entity_add(struct recs *recs);
void recs_entity_remove(struct recs *recs, entity e);
void recs_entity_remove_at_id_index(struct recs *recs, uint32_t id_index);

entity recs_entity_get(struct recs *recs, uint32_t index);


void recs_entity_add_component(struct recs *recs, entity e, component_type comp_type, void *component);
void recs_entity_add_tag(struct recs *recs, entity e, tag_id comp_type);

void recs_entity_remove_component(struct recs *recs, entity e, component_type comp_type);
void recs_entity_remove_tag(struct recs *recs, entity e, tag_id id);
void recs_entity_remove_all_components(struct recs *recs, entity e);

int recs_entity_has_component(struct recs *recs, entity e, component_type c);
int recs_entity_has_tag(struct recs *recs, entity e, tag_id c);

void* recs_entity_get_component(struct recs *recs, entity e, component_type c);




#endif// RECS_H
