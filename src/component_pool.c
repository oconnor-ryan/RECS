#include "component_pool.h"

#define NO_COMP_ID RECS_NO_ENTITY_ID


void component_pool_init(struct component_pool *ca, unsigned char *buffer, uint32_t component_size, uint32_t max_components, uint32_t max_entities) {
  ca->num_components = 0;
  ca->component_size = component_size;
  ca->max_components = max_components;

  size_t comp_buffer_size = component_size * max_components;
  size_t ent_to_comp_buffer_size = sizeof(uint32_t) * max_entities;




  unsigned char *comp_buffer = buffer;
  unsigned char *ent_to_comp_buffer = buffer + comp_buffer_size;
  unsigned char *comp_to_ent_buffer = buffer + comp_buffer_size + ent_to_comp_buffer_size;

  ca->buffer = (char*)comp_buffer;
  ca->comp_to_entity = (uint32_t*)comp_to_ent_buffer;
  ca->entity_to_comp = (uint32_t*) ent_to_comp_buffer;

  //mark all components as not belonging to any entity. 
  //Because this game will never get to a point where there are 65000 entities or
  //components, using a really big number as a marker for a non-existant component/entity
  //is perfectly find. 

  for(uint32_t i = 0; i < ca->max_components; i++) {
    ca->comp_to_entity[i] = RECS_NO_ENTITY_ID;
  }

  for(uint32_t i = 0; i < max_entities; i++) {
    ca->entity_to_comp[i] = NO_COMP_ID;
  }
  
}

void *component_pool_get(struct component_pool *ca, recs_entity e) {
  
  uint32_t component_index = ca->entity_to_comp[RECS_ENT_ID(e)];

  if(component_index == NO_COMP_ID) {
    return NULL;
  }
  return ca->buffer + (ca->component_size * component_index);
}


void component_pool_add(struct component_pool *ca, recs_entity e, void *component) {
  RECS_ASSERT(ca->num_components < ca->max_components);

  uint32_t component_index = ca->num_components;

  memcpy(ca->buffer + (ca->component_size * component_index), component, ca->component_size);

  ca->comp_to_entity[component_index] = RECS_ENT_ID(e);
  ca->entity_to_comp[RECS_ENT_ID(e)] = component_index;

  ca->num_components++;

}

void component_pool_remove(struct component_pool *ca, recs_entity e) {
  uint32_t component_index = ca->entity_to_comp[RECS_ENT_ID(e)];

  if(component_index == NO_COMP_ID) {
    return;
  }
  uint32_t last_component_index = ca->num_components-1;
  recs_entity entity_at_last_component = ca->comp_to_entity[last_component_index];


  //move last element to component being removed.
  //this keeps our list of components contiguous.
  //NOTE: Whether this has any performance benefits over allowing
  //our component pool buffer to be sparse is not tested.
  memcpy(
    ca->buffer + (ca->component_size * component_index),
    ca->buffer + (ca->component_size * last_component_index),
    ca->component_size
  );

  ca->comp_to_entity[component_index] = entity_at_last_component;
  ca->entity_to_comp[entity_at_last_component] = component_index;

  ca->comp_to_entity[last_component_index] = RECS_ENT_ID(e);
  ca->entity_to_comp[RECS_ENT_ID(e)] = NO_COMP_ID;

  ca->num_components--;


}

