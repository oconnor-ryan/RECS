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
