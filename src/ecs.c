#include "ecs.h"

#include "def.h"


//all tags appear AFTER the component_types with data.
static inline uint32_t ecs_tag_id_to_comp_id(struct ecs *ecs, tag_id t) {
  return t + ecs->max_component_types;
}

int ecs_init(struct ecs *ecs, uint32_t max_entities, uint32_t max_component_types, uint32_t max_tags, uint32_t max_systems) {
  if(!entity_manager_init(&ecs->ent_man, max_entities)) {
    return 0;
  }

  //lots of allocations. Figure out how to combine multiple allocations into
  //one big allocation.

  ecs->max_systems = max_systems;
  ecs->max_component_types = max_component_types;
  ecs->max_tags = max_tags;

  ecs->num_registered_components = 0;
  ecs->num_registered_systems = 0;
  ecs->num_registered_tags = 0;

  uint32_t max_comps = max_component_types + max_tags;

  //assert that max_comps does not overflow
  ASSERT(max_comps > max_component_types && max_comps > max_tags);

  //assert that we don't have 2^32 - 1 entities, since the largest 32-bit unsigned
  //integer is used as a marker for something with no entities
  ASSERT(max_entities != NO_ENTITY_ID);
  

  size_t size_of_bitset_for_1 = 1 + ((max_comps) / 8);

  char *bitmask_buffer = MALLOC(max_entities * size_of_bitset_for_1);
  if(bitmask_buffer == NULL) {
    entity_manager_free(&ecs->ent_man);
    return 0;
  }
  ecs->bitmask_byte_buffer = bitmask_buffer;

  ecs->entity_component_bitmask = MALLOC(max_entities * sizeof(struct bitset));
  if(ecs->entity_component_bitmask == NULL) {
    entity_manager_free(&ecs->ent_man);
    FREE(ecs->bitmask_byte_buffer);
    return 0;
  }

  for(uint64_t i = 0; i < max_entities; i++) {
    uint8_t *buffer_index = (uint8_t*)ecs->bitmask_byte_buffer + (i * size_of_bitset_for_1);
    ecs->entity_component_bitmask[i] = bitset_init(buffer_index, max_comps); 
    bitset_set_all(&ecs->entity_component_bitmask[i], 0);
  }

  //init component pools (dont add pools for tags, since they hold 0 data)

  ecs->component_type_stores = MALLOC(ecs->max_component_types * sizeof(struct component_pool));

  if(ecs->component_type_stores == NULL) {
    entity_manager_free(&ecs->ent_man);
    FREE(ecs->entity_component_bitmask);
    FREE(ecs->bitmask_byte_buffer);
    return 0;
  }

  ecs->systems = MALLOC(ecs->max_systems * sizeof(struct ecs_system));
  if(ecs->systems == NULL) {
    entity_manager_free(&ecs->ent_man);
    FREE(ecs->entity_component_bitmask);
    FREE(ecs->component_type_stores);
    FREE(ecs->bitmask_byte_buffer);

    return 0;
  }


  return 1;
  
}

void ecs_free(struct ecs *ecs) {
  //remember that we make 1 BIG allocation to store all bitsets
  FREE(ecs->bitmask_byte_buffer);
  FREE(ecs->entity_component_bitmask);

  entity_manager_free(&ecs->ent_man);

  for(uint32_t i = 0; i < ecs->num_registered_components; i++) {
    component_pool_free(ecs->component_type_stores + i);
  }

  FREE(ecs->systems);

  FREE(ecs->component_type_stores);

}

int ecs_component_register(struct ecs *ecs, component_type type, uint32_t max_components, size_t comp_size) {
  ASSERT(comp_size > 0);

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
  bitset_set(&ecs->entity_component_bitmask[e], comp_type, 1);
}

void ecs_entity_add_tag(struct ecs *ecs, entity e, tag_id tag) {
  bitset_set(
    &ecs->entity_component_bitmask[e], 
    ecs_tag_id_to_comp_id(ecs, tag), 
    1
  );
}

void ecs_entity_remove_component(struct ecs *ecs, entity e, component_type comp_type) {
  struct component_pool *ca =  ecs->component_type_stores + comp_type;
  component_pool_remove(ca, e);

  //clear bit
  bitset_set(&ecs->entity_component_bitmask[e], comp_type, 0);

}

void ecs_entity_remove_tag(struct ecs *ecs, entity e, tag_id tag) {
  bitset_set(
    &ecs->entity_component_bitmask[e], 
    ecs_tag_id_to_comp_id(ecs, tag), 
    0
  );
}

void ecs_entity_remove_all_components(struct ecs *ecs, entity e) {

  //remove components from component arrays
  for(component_type t = 0; t < ecs->max_component_types; t++) {
    if(bitset_test(&ecs->entity_component_bitmask[e], t)) {
      ecs_entity_remove_component(ecs, e, t);
    }
  }

  //mark entity as having no components to clear tags
  bitset_set_all(&ecs->entity_component_bitmask[e], 0);

}

int ecs_entity_has_component(struct ecs *ecs, entity e, component_type c) {
  return bitset_test(&ecs->entity_component_bitmask[e], c);
}

int ecs_entity_has_tag(struct ecs *ecs, entity e, tag_id c) {
  return bitset_test(&ecs->entity_component_bitmask[e], ecs_tag_id_to_comp_id(ecs, c));
}

void* ecs_entity_get_component(struct ecs *ecs, entity e, component_type c) {
  return component_pool_get(ecs->component_type_stores + c, e);
}

struct component_pool* ecs_get_component_pool(struct ecs *ecs, component_type c) {
  return ecs->component_type_stores + c;
}
