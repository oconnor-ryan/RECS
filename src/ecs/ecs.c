#include "recs.h"

#include "ecs/bitmask_handler.h"
#include "ecs/component_manager.h"
#include "ecs/entity_manager.h"



struct recs_system {
  recs_system_func func;
};


//marks where groups of systems start at within our systems buffer.
struct system_group_mapper {
  uint32_t num_systems;
  uint32_t starting_index;
};

struct recs {
  struct component_pool *recs_component_stores;

  uint32_t num_registered_components;
  uint32_t num_registered_systems;

  uint32_t max_recs_components;
  uint32_t max_tags;
  uint32_t max_systems;

  uint32_t num_system_types;
  uint32_t max_system_types;


  //used to know what components each entity has
  struct comp_bitmask_list comp_bitmasks;

  //a user provided pointer that contains extra data about their app state that needs to be seen/modified by
  //an ECS system.
  void *system_context; 

  struct recs_system *systems;
  struct system_group_mapper *system_group_mappers;

  struct entity_manager ent_man;

};

//all tags appear AFTER the recs_components with data.
static inline uint32_t recs_tag_id_to_comp_id(struct recs *ecs, recs_tag tag) {
  uint32_t id = tag + ecs->max_recs_components;
  RECS_ASSERT(id < ecs->max_tags + ecs->max_recs_components);
  return id;
}



uint32_t recs_num_active_entities(struct recs *recs) {
  return recs->ent_man.num_active_entities;
}

uint32_t recs_max_entities(struct recs *recs) {
  return recs->ent_man.max_entities;
}


recs recs_init(uint32_t max_entities, uint32_t max_recs_components, uint32_t max_tags, uint32_t max_systems, uint32_t max_system_groups, void *context) {

  uint32_t max_comps = max_recs_components + max_tags;

  //RECS_ASSERT that max_comps does not overflow
  RECS_ASSERT(max_comps > max_recs_components && max_comps > max_tags);

  //RECS_ASSERT that we don't have 2^32 - 1 entities, since the largest 32-bit unsigned
  //integer is used as a marker for something with no entities
  RECS_ASSERT(max_entities != RECS_NO_ENTITY_ID);





  //get sizes needed for each buffer
  size_t recs_buffer_size = sizeof(struct recs);
  size_t entity_buffer_size = sizeof(recs_entity) * max_entities;

  //note that size includes padding so that each bitset is byte-addressable.
  // So if each bitmask requires 4 bits, 1 byte will be allocated to each 
  // bitmask, leaving the last 4 bits of each bitmask used for padding.
  size_t size_per_bitmask = 1 + ((max_comps) / 8);
  size_t bitmask_list_buffer_size = max_entities * size_per_bitmask;
  size_t component_pool_list_size = max_recs_components * sizeof(struct component_pool);
  size_t system_buffer_size = max_systems * sizeof(struct recs_system);
  size_t system_mapper_size = max_system_groups * sizeof(struct system_group_mapper);

  //allocate one big buffer that will store nearly all of the ECS's dynamically allocated data.
  uint8_t *big_buffer = RECS_MALLOC(recs_buffer_size + entity_buffer_size + bitmask_list_buffer_size + component_pool_list_size + system_buffer_size + system_mapper_size);
  if(big_buffer == NULL) {
    return NULL;
  }

  recs ecs = (recs) big_buffer;
  uint8_t *entity_buffer =         big_buffer + recs_buffer_size;
  uint8_t *bitmask_buffer =        big_buffer + recs_buffer_size + entity_buffer_size;
  uint8_t *component_pool_buffer = big_buffer + recs_buffer_size + entity_buffer_size + bitmask_list_buffer_size;
  uint8_t *system_buffer =         big_buffer + recs_buffer_size + entity_buffer_size + bitmask_list_buffer_size + component_pool_list_size;
  uint8_t *system_mapper_buffer =  big_buffer + recs_buffer_size + entity_buffer_size + bitmask_list_buffer_size + component_pool_list_size + system_buffer_size;



  //assign all buffers to appropriate parts of ECS.

  ecs->max_systems = max_systems;
  ecs->max_recs_components = max_recs_components;
  ecs->max_tags = max_tags;

  ecs->num_registered_components = 0;
  ecs->num_registered_systems = 0;
  ecs->max_system_types = max_system_groups;
  ecs->system_context = context;

  entity_manager_init(&ecs->ent_man, (recs_entity*)entity_buffer, max_entities);

  comp_bitmask_list_init(&ecs->comp_bitmasks, bitmask_buffer, ecs->ent_man.max_entities, max_comps);
  comp_bitmask_list_clear(&ecs->comp_bitmasks, 0);

  ecs->recs_component_stores = (struct component_pool*)component_pool_buffer;
  ecs->systems = (struct recs_system*)system_buffer;
  ecs->system_group_mappers = (struct system_group_mapper*)system_mapper_buffer;

  //initialize each mapper with 0 systems by default
  for(uint32_t i = 0; i < max_system_groups; i++) {
    ecs->system_group_mappers[i].num_systems = 0;
  }

  return ecs;
  
}

void recs_free(struct recs *ecs) {
  if(ecs == NULL) {
    return;
  }

  //make sure to free individual component pools before freeing the rest
  //of the ECS buffer
  
  //because each registered component pool makes its own allocation
  //to fit the variable component sizes, we need to free them individually.
  for(uint32_t i = 0; i < ecs->num_registered_components; i++) {
    component_pool_free(ecs->recs_component_stores + i);
  }


  //remember that we made 1 BIG allocation to store most data
  RECS_FREE(ecs);

  
}

int recs_component_register(struct recs *ecs, recs_component type, uint32_t max_components, size_t comp_size) {
  RECS_ASSERT(comp_size > 0);
  RECS_ASSERT(ecs->num_registered_components < ecs->max_recs_components);

  //register component with attached data
  struct component_pool *cp = ecs->recs_component_stores + type;



  if(!component_pool_init(cp, comp_size, max_components, ecs->ent_man.max_entities)) {
    return 0;
  }

  ecs->num_registered_components++;

  return 1;
}




void recs_system_register(struct recs *ecs, recs_system_func func, recs_system_group group) {
  RECS_ASSERT(ecs->num_registered_systems < ecs->max_systems);


  //each type we register a new system, we need to maintain the correct order of each system such that they are 
  //placed contiguously with systems within the same group id, and that they stay in the order they are registered in.

  //struct recs_system *s = ecs->systems + ecs->num_registered_systems;
  //s->func = func;

  struct system_group_mapper *m = ecs->system_group_mappers + group;

  //this is a new group, set the mapper and place system at end of system list
  if(m->num_systems == 0) {
    m->num_systems = 1;
    m->starting_index = ecs->num_registered_systems;
    struct recs_system *s = ecs->systems + ecs->num_registered_systems;
    s->func = func;
  } else {
    uint32_t system_index = m->starting_index + m->num_systems;

    //shift all systems to right starting at current group's last index.
    for(uint32_t i = ecs->num_registered_systems; i > system_index; i--) {
      ecs->systems[i] = ecs->systems[i-1];
    }

    //update all mappers with starting index >= system_index to increment starting_index by 1
    for(uint32_t i = 0; i < ecs->max_system_types; i++) {
      struct system_group_mapper *map = ecs->system_group_mappers + i;
      if(map->starting_index >= system_index && map->num_systems != 0) {
        map->starting_index++;
      }
    }


    //place new system at correct index
    ecs->systems[system_index].func = func;

    //update mapper
    m->num_systems++;

  }

  ecs->num_registered_systems++;
  
}

void recs_system_set_context(struct recs *ecs, void *context) {
  ecs->system_context = context;
}

void* recs_system_get_context(struct recs *ecs) {
  return ecs->system_context;
}


void recs_system_run(struct recs *ecs, recs_system_group type) {
  uint32_t system_group_start_index = ecs->system_group_mappers[type].starting_index;
  uint32_t num_systems = ecs->system_group_mappers[type].num_systems;
  for(uint32_t i = 0; i < num_systems; i++) {
    ecs->systems[system_group_start_index + i].func(ecs);
  }
}

recs_entity recs_entity_get(struct recs *ecs, uint32_t index) {
  return ecs->ent_man.set_of_ids[index];
}

recs_entity recs_entity_add(struct recs *ecs) {
  RECS_ASSERT(ecs->ent_man.num_active_entities < ecs->ent_man.max_entities);

  recs_entity e = entity_manager_add(&ecs->ent_man);
  return e;
  
}

void recs_entity_remove(struct recs *ecs, recs_entity e) {


  recs_entity_remove_all_components(ecs, e);


  //return ID to be reused by entity manager
  entity_manager_remove(&ecs->ent_man, e);
}

void recs_entity_remove_at_id_index(struct recs *ecs, uint32_t id_index) {
  recs_entity e = ecs->ent_man.set_of_ids[id_index];
  recs_entity_remove_all_components(ecs, e);
  entity_manager_remove_at_index(&ecs->ent_man, id_index);
}


void recs_entity_add_component(struct recs *ecs, recs_entity e, recs_component comp_type, void *component) {

  struct component_pool *ca = ecs->recs_component_stores + comp_type;
  component_pool_add(ca, e, component);

  //set bit
  comp_bitmask_list_set(&ecs->comp_bitmasks, e, comp_type, 1);
}

void recs_entity_add_tag(struct recs *ecs, recs_entity e, recs_tag tag) {
  comp_bitmask_list_set(
    &ecs->comp_bitmasks, 
    e,
    recs_tag_id_to_comp_id(ecs, tag), 
    1
  );
}

void recs_entity_remove_component(struct recs *ecs, recs_entity e, recs_component comp_type) {
  struct component_pool *ca =  ecs->recs_component_stores + comp_type;
  component_pool_remove(ca, e);

  //clear bit
  comp_bitmask_list_set(&ecs->comp_bitmasks, e, comp_type, 0);

}

void recs_entity_remove_tag(struct recs *ecs, recs_entity e, recs_tag tag) {
  comp_bitmask_list_set(
    &ecs->comp_bitmasks,
    e, 
    recs_tag_id_to_comp_id(ecs, tag), 
    0
  );
}

void recs_entity_remove_all_components(struct recs *ecs, recs_entity e) {

  //remove components from component arrays
  for(recs_component t = 0; t < ecs->max_recs_components; t++) {
    if(comp_bitmask_list_test(&ecs->comp_bitmasks, e, t)) {
      recs_entity_remove_component(ecs, e, t);
    }
  }

  //mark entity as having no components to clear tags
  comp_bitmask_list_set_all_in_mask(&ecs->comp_bitmasks, e, 0);

}

int recs_entity_has_component(struct recs *ecs, recs_entity e, recs_component c) {
  return comp_bitmask_list_test(&ecs->comp_bitmasks, e, c);
}

int recs_entity_has_tag(struct recs *ecs, recs_entity e, recs_tag tag) {
  return comp_bitmask_list_test(&ecs->comp_bitmasks, e, recs_tag_id_to_comp_id(ecs, tag));
}

void* recs_entity_get_component(struct recs *ecs, recs_entity e, recs_component c) {
  return component_pool_get(ecs->recs_component_stores + c, e);
}

//components are densely packed, so you can retrieve them using an index
//if desired. Note that components will not stay at the same index when removing
//components, so make sure not to remove components when using this function
void* recs_component_get(struct recs *recs, recs_component c, uint32_t index) {
  struct component_pool *p = recs->recs_component_stores + c;
  return p->buffer + (index * p->component_size);
}

int recs_entity_has_components(struct recs *recs, recs_entity e, uint32_t num_comps, recs_component *c) {
  for(uint32_t i = 0; i < num_comps; i++) {
    if(!recs_entity_has_component(recs, e, c[i])) {
      return 0;
    }
  }
  return 1;
}

int recs_entity_has_tags(struct recs *recs, recs_entity e, uint32_t num_tags, recs_tag *t) {
  for(uint32_t i = 0; i < num_tags; i++) {
    if(!recs_entity_has_tag(recs, e, t[i])) {
      return 0;
    }
  }
  return 1;
}



recs_entity recs_entity_get_with_comps(struct recs *ecs, uint32_t num_comps, recs_component *comp_ids, uint32_t num_tags, recs_tag *tags, uint32_t *index) {
  for(; (*index) < ecs->ent_man.num_active_entities; (*index)++) {
    recs_entity e = ecs->ent_man.set_of_ids[*index];
    if(recs_entity_has_components(ecs, e, num_comps, comp_ids) && recs_entity_has_tags(ecs, e, num_tags, tags)) {
      (*index)++;
      return e;
    }
  }
  return RECS_NO_ENTITY_ID;
}

