#ifndef ECS_COMPONENT_MANAGER
#define ECS_COMPONENT_MANAGER

#include "entity_manager.h"

typedef uint32_t component_type;

struct component_pool {
  char *buffer;
  uint32_t component_size;

  uint32_t num_components;
  uint32_t max_components;

  //in future, we can consider using a dynamic array to save memory,
  //though I believe that we won't have enough components to cause a big issue.
  uint32_t *entity_to_comp;
  uint32_t *comp_to_entity;

};


int component_pool_init(struct component_pool *ca, uint32_t component_size, uint32_t max_components, uint32_t max_entities);
void *component_pool_get(struct component_pool *ca, entity e);
void component_pool_copy(struct component_pool *ca, entity e, void *component);
void component_pool_add(struct component_pool *ca, entity e, void *component);
void component_pool_remove(struct component_pool *ca, entity e);
void component_pool_free(struct component_pool *ca);





#endif// ECS_COMPONENT_MANAGER
