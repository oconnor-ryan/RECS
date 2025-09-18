#ifndef ECS_COMPONENT_MANAGER
#define ECS_COMPONENT_MANAGER

#include "entity_manager.h"
#include "recs.h"


struct component_pool {
  char *buffer;
  uint32_t component_size;

  uint32_t num_components;
  uint32_t max_components;

  //rather than implementing a hash map, we will use 
  //arrays to map entity IDs to component indexes.
  uint32_t *entity_to_comp;

  //this is needed since when adding/removing components, we keep 
  //component data contiguous by moving the last component into the component
  //being removed. This requires us to keep track of each component->entity mapping
  //so that we can properly update our entity->comp mapping.
  recs_entity *comp_to_entity;

};


int component_pool_init(struct component_pool *ca, uint32_t component_size, uint32_t max_components, uint32_t max_entities);
void *component_pool_get(struct component_pool *ca, recs_entity e);
void component_pool_add(struct component_pool *ca, recs_entity e, void *component);
void component_pool_remove(struct component_pool *ca, recs_entity e);
void component_pool_free(struct component_pool *ca);





#endif// ECS_COMPONENT_MANAGER
