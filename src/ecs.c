#include "ecs.h"

#include "def.h"


//all tags appear AFTER the component_types with data.
static inline uint32_t ecs_tag_id_to_comp_id(struct ecs *ecs, tag_id t) {
  uint32_t id = t + ecs->max_component_types;
  ASSERT(id < ecs->max_tags + ecs->max_component_types);
  return id;
}


int ecs_init(struct ecs *ecs, uint32_t max_entities, uint32_t max_component_types, uint32_t max_tags, uint32_t max_systems) {
  uint32_t max_comps = max_component_types + max_tags;

  //assert that max_comps does not overflow
  ASSERT(max_comps > max_component_types && max_comps > max_tags);

  //assert that we don't have 2^32 - 1 entities, since the largest 32-bit unsigned
  //integer is used as a marker for something with no entities
  ASSERT(max_entities != NO_ENTITY_ID);


  ecs->max_systems = max_systems;
  ecs->max_component_types = max_component_types;
  ecs->max_tags = max_tags;

  ecs->num_registered_components = 0;
  ecs->num_registered_systems = 0;
  ecs->num_registered_tags = 0;


  //get sizes needed for each buffer
  size_t entity_buffer_size = sizeof(entity) * max_entities;

  //note that size includes padding so that each bitset is byte-addressable.
  // So if each bitmask requires 4 bits, 1 byte will be allocated to each 
  // bitmask, leaving the last 4 bits of each bitmask used for padding.
  size_t size_per_bitmask = 1 + ((max_comps) / 8);
  size_t bitmask_list_buffer_size = max_entities * size_per_bitmask;
  size_t component_pool_list_size = max_component_types * sizeof(struct component_pool);
  size_t system_buffer_size = ecs->max_systems * sizeof(struct ecs_system);

  //allocate one big buffer that will store nearly all of the ECS's dynamically allocated data.
  uint8_t *big_buffer = MALLOC(entity_buffer_size + bitmask_list_buffer_size + component_pool_list_size + system_buffer_size);
  if(big_buffer == NULL) {
    return 0;
  }

  uint8_t *entity_buffer =         big_buffer;
  uint8_t *bitmask_buffer =        big_buffer + entity_buffer_size;
  uint8_t *component_pool_buffer = big_buffer + entity_buffer_size + bitmask_list_buffer_size;
  uint8_t *system_buffer =         big_buffer + entity_buffer_size + bitmask_list_buffer_size + component_pool_list_size;


  //assign all buffers to appropriate parts of ECS.
  ecs->buffer = big_buffer; //makes it easier to deallocate since we know where our MALLOCED memory starts

  entity_manager_init(&ecs->ent_man, (entity*)entity_buffer, max_entities);

  comp_bitmask_list_init(&ecs->comp_bitmasks, bitmask_buffer, ecs->ent_man.max_entities, max_comps);
  comp_bitmask_list_clear(&ecs->comp_bitmasks, 0);

  ecs->component_type_stores = (struct component_pool*)component_pool_buffer;
  ecs->systems = (struct ecs_system*)system_buffer;

  return 1;
  
}

void ecs_free(struct ecs *ecs) {
  //remember that we make 1 BIG allocation to store all bitsets
  FREE(ecs->buffer);

  //because each registered component pool makes its own allocation
  //to fit the variable component sizes, we need to free them individually.
  for(uint32_t i = 0; i < ecs->num_registered_components; i++) {
    component_pool_free(ecs->component_type_stores + i);
  }
}

int ecs_component_register(struct ecs *ecs, component_type type, uint32_t max_components, size_t comp_size) {
  ASSERT(comp_size > 0);
  ASSERT(ecs->num_registered_components < ecs->max_component_types);

  //register component with attached data
  struct component_pool *cp = ecs->component_type_stores + type;



  if(!component_pool_init(cp, comp_size, max_components, ecs->ent_man.max_entities)) {
    return 0;
  }

  ecs->num_registered_components++;

  return 1;
}




void ecs_system_register(struct ecs *ecs, ecs_system_func func, void *context, enum ecs_system_type type) {

  struct ecs_system *s = ecs->systems + ecs->num_registered_systems;
  s->context = context;
  s->func = func;
  s->type = type;

  ecs->num_registered_systems++;
  
}

void ecs_system_set_context(struct ecs *ecs, uint32_t system_index, void *context) {
  ecs->systems[system_index].context = context;
}

void ecs_system_run(struct ecs *ecs, uint32_t system_index) {
  ecs->systems[system_index].func(ecs, ecs->systems[system_index].context);
}

void ecs_system_run_all_with_type(struct ecs *ecs, enum ecs_system_type type) {
  for(uint32_t i = 0; i < ecs->num_registered_systems; i++) {
    struct ecs_system *s = ecs->systems + i;
    if(s->type == type) {
      s->func(ecs, s->context);
    }
  }
}

entity ecs_entity_get(struct ecs *ecs, uint32_t index) {
  return ecs->ent_man.set_of_ids[index];
}

entity ecs_entity_add(struct ecs *ecs) {
  ASSERT(ecs->ent_man.num_active_entities < ecs->ent_man.max_entities);

  entity e = entity_manager_add(&ecs->ent_man);
  return e;
  
}

void ecs_entity_remove(struct ecs *ecs, entity e) {


  ecs_entity_remove_all_components(ecs, e);


  //return ID to be reused by entity manager
  entity_manager_remove(&ecs->ent_man, e);
}

void ecs_entity_remove_at_id_index(struct ecs *ecs, uint32_t id_index) {
  entity e = ecs->ent_man.set_of_ids[id_index];
  ecs_entity_remove_all_components(ecs, e);
  entity_manager_remove_at_index(&ecs->ent_man, id_index);
}


void ecs_entity_add_component(struct ecs *ecs, entity e, component_type comp_type, void *component) {

  struct component_pool *ca = ecs->component_type_stores + comp_type;
  component_pool_add(ca, e, component);

  //set bit
  comp_bitmask_list_set(&ecs->comp_bitmasks, e, comp_type, 1);
}

void ecs_entity_add_tag(struct ecs *ecs, entity e, tag_id tag) {
  comp_bitmask_list_set(
    &ecs->comp_bitmasks, 
    e,
    ecs_tag_id_to_comp_id(ecs, tag), 
    1
  );
}

void ecs_entity_remove_component(struct ecs *ecs, entity e, component_type comp_type) {
  struct component_pool *ca =  ecs->component_type_stores + comp_type;
  component_pool_remove(ca, e);

  //clear bit
  comp_bitmask_list_set(&ecs->comp_bitmasks, e, comp_type, 0);

}

void ecs_entity_remove_tag(struct ecs *ecs, entity e, tag_id tag) {
  comp_bitmask_list_set(
    &ecs->comp_bitmasks,
    e, 
    ecs_tag_id_to_comp_id(ecs, tag), 
    0
  );
}

void ecs_entity_remove_all_components(struct ecs *ecs, entity e) {

  //remove components from component arrays
  for(component_type t = 0; t < ecs->max_component_types; t++) {
    if(comp_bitmask_list_test(&ecs->comp_bitmasks, e, t)) {
      ecs_entity_remove_component(ecs, e, t);
    }
  }

  //mark entity as having no components to clear tags
  comp_bitmask_list_set_all_in_mask(&ecs->comp_bitmasks, e, 0);

}

int ecs_entity_has_component(struct ecs *ecs, entity e, component_type c) {
  return comp_bitmask_list_test(&ecs->comp_bitmasks, e, c);
}

int ecs_entity_has_tag(struct ecs *ecs, entity e, tag_id c) {
  return comp_bitmask_list_test(&ecs->comp_bitmasks, e, ecs_tag_id_to_comp_id(ecs, c));
}

void* ecs_entity_get_component(struct ecs *ecs, entity e, component_type c) {
  return component_pool_get(ecs->component_type_stores + c, e);
}

struct component_pool* ecs_get_component_pool(struct ecs *ecs, component_type c) {
  return ecs->component_type_stores + c;
}
