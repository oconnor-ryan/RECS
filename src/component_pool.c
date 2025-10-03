#include "component_pool.h"

#define NO_COMP_ID RECS_NO_ENTITY_ID


int component_pool_init(struct component_pool *ca, uint32_t component_size, uint32_t max_components, uint32_t max_entities) {
  ca->num_components = 0;
  ca->component_size = component_size;
  ca->max_components = max_components;

  //allocate buffer
  size_t comp_buffer_size = component_size * max_components;

  size_t ent_to_comp_buffer_size = sizeof(uint32_t) * max_entities;

  //only allocate to max_components since that is usually equal to 
  //or less than the max_entities, making memory storage slightly more efficient.
  size_t comp_to_ent_buffer_size = sizeof(recs_entity) * max_components;


  //allocate all memory at once needed to store raw component data,
  //entity to component mapper, and component to entity mapper.
  char *buffer = (char*)RECS_MALLOC(comp_buffer_size + ent_to_comp_buffer_size + comp_to_ent_buffer_size);
  if(buffer == NULL) {
    return 0;
  }
  char *comp_buffer = buffer;
  char *ent_to_comp_buffer = buffer + comp_buffer_size;
  char *comp_to_ent_buffer = buffer + comp_buffer_size + ent_to_comp_buffer_size;

  ca->buffer = comp_buffer;
  ca->comp_to_entity = (uint32_t*)comp_to_ent_buffer;
  ca->entity_to_comp = (recs_entity*) ent_to_comp_buffer;

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
  
  return 1;
}

void *component_pool_get(struct component_pool *ca, recs_entity e) {
  
  uint32_t component_index = ca->entity_to_comp[e];

  if(component_index == NO_COMP_ID) {
    return NULL;
  }
  return ca->buffer + (ca->component_size * component_index);
}


void component_pool_add(struct component_pool *ca, recs_entity e, void *component) {
  RECS_ASSERT(ca->num_components < ca->max_components);

  uint32_t component_index = ca->num_components;

  memcpy(ca->buffer + (ca->component_size * component_index), component, ca->component_size);

  ca->comp_to_entity[component_index] = e;
  ca->entity_to_comp[e] = component_index;

  ca->num_components++;

}

void component_pool_remove(struct component_pool *ca, recs_entity e) {
  uint32_t component_index = ca->entity_to_comp[e];

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

  ca->comp_to_entity[last_component_index] = e;
  ca->entity_to_comp[e] = NO_COMP_ID;

  ca->num_components--;


}

void component_pool_free(struct component_pool *ca) {
  //remember we made 1 big allocation starting at ca->buffer,
  //so we only need to RECS_FREE that one buffer.
  RECS_FREE(ca->buffer);
  //RECS_FREE(ca->comp_to_entity);
  //RECS_FREE(ca->entity_to_comp);
}
