#ifndef ECS_H
#define ECS_H

#include "component_manager.h"

#include "entity_manager.h"

#include "bitmask_handler.h"

#include <stddef.h>

typedef uint32_t tag_id;


//because game updates/physics may run on a separate time-step than
//game renders/draws, we will allow the option to specify which set
//of systems should run.

enum ecs_system_type {
  ECS_SYSTEM_TYPE_UPDATE,
  ECS_SYSTEM_TYPE_RENDER,

};

//forward declare
struct ecs;

typedef void (*ecs_system_func)(struct ecs *ecs, void* context);

struct ecs_system {
  ecs_system_func func;
  void *context;
  enum ecs_system_type type;
};




struct ecs {
  struct component_pool *component_type_stores;

  uint32_t num_registered_components;
  uint32_t num_registered_tags;
  uint32_t num_registered_systems;

  uint32_t max_component_types;
  uint32_t max_tags;
  uint32_t max_systems;

  void *buffer;

  //used to know what components each entity has
  struct comp_bitmask_list comp_bitmasks;

  struct ecs_system *systems;

  struct entity_manager ent_man;

};


/*
static inline uint8_t ecs_is_tag(struct ecs *ecs, component_type t) {
  return t >= ecs->max_component_types;
} 
*/

int ecs_init(struct ecs *ecs, uint32_t max_entities, uint32_t max_component_types, uint32_t max_tags, uint32_t max_systems);
void ecs_free(struct ecs *ecs);

int ecs_component_register(struct ecs *ecs, component_type type, uint32_t max_components, size_t comp_size);

void ecs_system_register(struct ecs *ecs, ecs_system_func func, void *context, enum ecs_system_type type);

void ecs_system_set_context(struct ecs *ecs, uint32_t system_index, void *context);
void ecs_system_run(struct ecs *ecs, uint32_t system_index);
void ecs_system_run_all_with_type(struct ecs *ecs, enum ecs_system_type type);




entity ecs_entity_add(struct ecs *ecs);
void ecs_entity_remove(struct ecs *ecs, entity e);
void ecs_entity_remove_at_id_index(struct ecs *ecs, uint32_t id_index);

entity ecs_entity_get(struct ecs *ecs, uint32_t index);


void ecs_entity_add_component(struct ecs *ecs, entity e, component_type comp_type, void *component);
void ecs_entity_add_tag(struct ecs *ecs, entity e, tag_id comp_type);

void ecs_entity_remove_component(struct ecs *ecs, entity e, component_type comp_type);
void ecs_entity_remove_tag(struct ecs *ecs, entity e, tag_id id);
void ecs_entity_remove_all_components(struct ecs *ecs, entity e);

int ecs_entity_has_component(struct ecs *ecs, entity e, component_type c);
int ecs_entity_has_tag(struct ecs *ecs, entity e, tag_id c);

void* ecs_entity_get_component(struct ecs *ecs, entity e, component_type c);
struct component_pool* ecs_get_component_pool(struct ecs *ecs, component_type c);






#endif// ECS_H
