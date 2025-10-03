#ifndef COMPONENT_POOL_H
#define COMPONENT_POOL_H

#include <stdint.h>
#include <string.h>
#include "recs.h"

/* 
  Component Pool Section

  Stores the raw data of every entity's components and maps entity IDs to components within its raw buffer.
  This also handles creating and freeing component pools.
*/

struct component_pool {
  char *buffer;
  uint32_t component_size;

  uint32_t num_components;
  uint32_t max_components;
  uint32_t max_entities;

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

#endif// COMPONENT_POOL_H

