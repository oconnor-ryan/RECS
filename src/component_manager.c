#include "component_manager.h"
#include "ecs.h"

#include <string.h>

#include <stdlib.h>

#include "def.h"


int component_pool_init(struct component_pool *ca, uint32_t component_size, uint32_t max_components, uint32_t max_entities) {
  ca->num_components = 0;
  ca->component_size = component_size;
  ca->max_components = max_components;

  //allocate buffer
  size_t comp_buffer_size = component_size * max_components;
  size_t ent_to_comp_buffer_size = sizeof(uint32_t) * max_entities;

  char *comp_buffer = MALLOC(comp_buffer_size);
  if(comp_buffer == NULL) {
    return 0;
  }
  uint32_t *ent_to_comp_buffer = MALLOC(ent_to_comp_buffer_size);
  if(ent_to_comp_buffer == NULL) {
    FREE(comp_buffer);
    return 0;
  }

  uint32_t *comp_to_ent_buffer = MALLOC(ent_to_comp_buffer_size);
  if(comp_to_ent_buffer == NULL) {
    FREE(comp_buffer);
    FREE(ent_to_comp_buffer);
    return 0;
  }

  ca->buffer = comp_buffer;
  ca->entity_to_comp = ent_to_comp_buffer;
  ca->comp_to_entity = comp_to_ent_buffer;

  //mark all components as not belonging to any entity. 
  //Because this game will never get to a point where there are 65000 entities or
  //components, using a really big number as a marker for a non-existant component/entity
  //is perfectly find. 

  for(uint32_t i = 0; i < ca->max_components; i++) {
    ca->comp_to_entity[i] = NO_ENTITY_ID;
    ca->entity_to_comp[i] = NO_ENTITY_ID;

  }
  
  return 1;
}

void *component_pool_get(struct component_pool *ca, entity e) {
  
  uint32_t component_index = ca->entity_to_comp[e];

  if(component_index == NO_ENTITY_ID) {
    return NULL;
  }
  return ca->buffer + (ca->component_size * component_index);
}

void component_pool_copy(struct component_pool *ca, entity e, void *component) {
  uint32_t component_index = ca->entity_to_comp[e];
  memcpy(component, ca->buffer + (ca->component_size * component_index), ca->component_size);
}

void component_pool_add(struct component_pool *ca, entity e, void *component) {
  uint32_t component_index = ca->num_components;

  memcpy(ca->buffer + (ca->component_size * component_index), component, ca->component_size);

  ca->comp_to_entity[component_index] = e;
  ca->entity_to_comp[e] = component_index;

  ca->num_components++;

}

void component_pool_remove(struct component_pool *ca, entity e) {
  uint32_t component_index = ca->entity_to_comp[e];

  if(component_index == NO_ENTITY_ID) {
    return;
  }
  uint32_t last_component_index = ca->num_components-1;
  entity entity_at_last_component = ca->comp_to_entity[last_component_index];


  //move last element to component being removed
  memcpy(
    ca->buffer + (ca->component_size * component_index),
    ca->buffer + (ca->component_size * last_component_index),
    ca->component_size
  );

  ca->comp_to_entity[component_index] = entity_at_last_component;
  ca->entity_to_comp[entity_at_last_component] = component_index;

  ca->comp_to_entity[last_component_index] = e;
  ca->entity_to_comp[e] = NO_ENTITY_ID;

  ca->num_components--;


}

void component_pool_free(struct component_pool *ca) {
  FREE(ca->buffer);
  FREE(ca->comp_to_entity);
  FREE(ca->entity_to_comp);
}

