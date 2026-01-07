#include "entity_manager.h"
#include "bitmask.h"
#include "bitmask_list.h"
#include "component_pool.h"

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

  uint32_t max_registered_components;
  uint32_t max_registered_systems;
  uint32_t max_system_groups;
  uint32_t max_tags;

  //used to track size of one bitmask.
  size_t comp_bitmask_size;

  //used to know what components each entity has
  struct bitmask_list comp_bitmask_list;

  //a user provided pointer that contains extra data about their app state that needs to be seen/modified by
  //an ECS system.
  void *system_context; 

  struct recs_system *systems;
  struct system_group_mapper *system_group_mappers;

  struct entity_manager ent_man;

};

//all tags appear AFTER the recs_components with data.
static inline uint32_t recs_tag_id_to_comp_id(struct recs *ecs, recs_tag tag) {
  uint32_t id = tag + ecs->max_registered_components;
  RECS_ASSERT(id < ecs->max_tags + ecs->max_registered_components);
  return id;
}



uint32_t recs_num_active_entities(struct recs *recs) {
  return recs->ent_man.num_active_entities;
}


recs recs_init(uint32_t max_entities, uint32_t max_components, uint32_t max_tags, uint32_t max_systems, uint32_t max_sys_groups, void *context) {


  //RECS_ASSERT that max_comps does not overflow
  RECS_ASSERT((max_components + max_tags) > max_components && (max_components + max_tags) > max_tags);

  //RECS_ASSERT that we don't have 2^32 - 1 entities, since the largest 32-bit unsigned
  //integer is used as a marker for something with no entities
  RECS_ASSERT(max_entities-1 != RECS_NO_ENTITY_ID);

  size_t bytes_per_bitmask = RECS_GET_BITMASK_SIZE(max_components, max_tags);




  //get sizes needed for each buffer
  size_t recs_buffer_size = sizeof(struct recs);
  size_t entity_id_buffer_size = sizeof(recs_entity) * max_entities;
  size_t entity_version_buffer_size = sizeof(uint32_t) * max_entities;
  size_t component_pool_buffer_size = sizeof(struct component_pool) * max_components;
  size_t bitmask_buffer_size = bytes_per_bitmask * max_entities;
  size_t system_buffer_size = sizeof(struct recs_system) * max_systems;
  size_t system_mapper_buffer_size = sizeof(struct system_group_mapper) * max_sys_groups;


  //allocate one big buffer that will store nearly all of the ECS's dynamically allocated data.
  uint8_t *big_buffer = (uint8_t*)RECS_MALLOC(recs_buffer_size + entity_id_buffer_size + entity_version_buffer_size + component_pool_buffer_size + bitmask_buffer_size + system_buffer_size + system_mapper_buffer_size);
  if(big_buffer == NULL) {
    return NULL;
  }

  recs ecs = (recs) big_buffer;

  uint8_t *entity_id_buffer =      big_buffer + recs_buffer_size;
  uint8_t *entity_version_buffer = big_buffer + recs_buffer_size + entity_id_buffer_size;
  uint8_t *component_pool_buffer = big_buffer + recs_buffer_size + entity_id_buffer_size + entity_version_buffer_size;
  uint8_t *bitmask_buffer =        big_buffer + recs_buffer_size + entity_id_buffer_size + entity_version_buffer_size + component_pool_buffer_size;
  uint8_t *system_buffer =         big_buffer + recs_buffer_size + entity_id_buffer_size + entity_version_buffer_size + component_pool_buffer_size + bitmask_buffer_size;
  uint8_t *system_mapper_buffer =  big_buffer + recs_buffer_size + entity_id_buffer_size + entity_version_buffer_size + component_pool_buffer_size + bitmask_buffer_size + system_buffer_size;


  ecs->max_registered_components = max_components;
  ecs->max_tags = max_tags;
  ecs->max_registered_systems = max_systems;
  ecs->comp_bitmask_size = bytes_per_bitmask;
  ecs->max_system_groups = max_sys_groups;

  ecs->num_registered_components = 0;
  ecs->num_registered_systems = 0;
  ecs->system_context = context;


  //assign all buffers to appropriate parts of ECS.
  entity_manager_init(&ecs->ent_man, entity_id_buffer, entity_version_buffer, max_entities);
  bitmask_list_init(&ecs->comp_bitmask_list, ecs->comp_bitmask_size, bitmask_buffer);
  for(uint32_t i = 0; i < max_entities; i++) {
    uint8_t *mask = bitmask_list_get(&ecs->comp_bitmask_list, i);
    bitmask_clear(mask, 0, bytes_per_bitmask);
  }
  ecs->recs_component_stores = (struct component_pool*) component_pool_buffer;
  ecs->systems = (struct recs_system*)system_buffer;
  ecs->system_group_mappers = (struct system_group_mapper*)system_mapper_buffer;

  //initialize each mapper with 0 systems by default
  for(uint32_t i = 0; i < max_sys_groups; i++) {
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
  RECS_ASSERT(ecs->num_registered_components < ecs->max_registered_components);

  //register component with attached data
  struct component_pool *cp = ecs->recs_component_stores + type;



  if(!component_pool_init(cp, comp_size, max_components, ecs->ent_man.max_entities)) {
    return 0;
  }

  ecs->num_registered_components++;

  return 1;
}


void recs_component_unregister(struct recs *ecs, recs_component type) {
  struct component_pool *cp = ecs->recs_component_stores + type;

  //iterate through each entity and update their bitmask
  for(uint32_t i = 0; i < ecs->ent_man.num_active_entities; i++) {
    recs_entity e = ecs->ent_man.entity_pool[i];
    bitmask_set(bitmask_list_get(&ecs->comp_bitmask_list, RECS_ENT_ID(e)), type, 0);
  }
  //free memory
  component_pool_free(cp);
}

uint32_t recs_component_num_instances(struct recs *recs, recs_component c) {
  struct component_pool *p = recs->recs_component_stores + c;
  return p->num_components;
}

recs_entity recs_component_get_entity(struct recs *recs, recs_component c, uint32_t comp_index) {
  struct component_pool *p = recs->recs_component_stores + c;
  uint32_t id = p->comp_to_entity[comp_index];
  if(id == RECS_NO_ENTITY_ID) {
    return RECS_ENT_FROM(RECS_NO_ENTITY_ID, 0);
  }
  return RECS_ENT_FROM(id, recs->ent_man.ent_versions_list[id]);
}


void recs_system_register(struct recs *ecs, recs_system_func func, recs_system_group group) {
  RECS_ASSERT(ecs->num_registered_systems < ecs->max_registered_systems);


  //each type we register a new system, we need to maintain the correct order of each system such that they are 
  //placed contiguously with systems within the same group id, and that they stay in the order they are registered in.

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
    for(uint32_t i = 0; i < ecs->max_system_groups; i++) {
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


recs_entity recs_entity_add(struct recs *ecs) {
  RECS_ASSERT(ecs->ent_man.num_active_entities < ecs->ent_man.max_entities);

  recs_entity e = entity_manager_add(&ecs->ent_man);
  return e;
  
}

void recs_entity_remove(struct recs *ecs, recs_entity e) {
  if(RECS_ENT_ID(e) == RECS_NO_ENTITY_ID) return;

  //update version number
  if(RECS_ENT_VERSION(e) == ecs->ent_man.ent_versions_list[RECS_ENT_ID(e)]) {
    ecs->ent_man.ent_versions_list[RECS_ENT_ID(e)]++;
  }



  //delete components
  recs_entity_remove_all_components(ecs, e);

  //remove from active entity pool
  entity_manager_remove(&ecs->ent_man, e);

}

void recs_entity_queue_remove(struct recs *ecs, recs_entity e) {
  ecs->ent_man.ent_versions_list[RECS_ENT_ID(e)]++;
}

void recs_entity_remove_queued(struct recs *ecs) {
  if(ecs->ent_man.num_active_entities == 0) return;

  //move in reversed order since the index does not need to be modified when removing entities
  for(uint32_t i = ecs->ent_man.num_active_entities; i > 0; i--) {
    recs_entity e = ecs->ent_man.entity_pool[i-1];
    if(recs_entity_active(ecs, e)) continue;

    //delete components
    recs_entity_remove_all_components(ecs, e);

    //remove from active entity pool
    entity_manager_remove_at_index(&ecs->ent_man, i-1);


  }
}


void recs_entity_add_component(struct recs *ecs, recs_entity e, recs_component comp_type, void *component) {

  struct component_pool *ca = ecs->recs_component_stores + comp_type;
  component_pool_add(ca, e, component);

  //set bit
  bitmask_set(bitmask_list_get(&ecs->comp_bitmask_list, e), comp_type, 1);
}

void recs_entity_add_tag(struct recs *ecs, recs_entity e, recs_tag tag) {
  bitmask_set(bitmask_list_get(&ecs->comp_bitmask_list, RECS_ENT_ID(e)), recs_tag_id_to_comp_id(ecs, tag), 1);
}

void recs_entity_remove_component(struct recs *ecs, recs_entity e, recs_component comp_type) {
  struct component_pool *ca =  ecs->recs_component_stores + comp_type;
  component_pool_remove(ca, e);

  //clear bit
  bitmask_set(bitmask_list_get(&ecs->comp_bitmask_list, RECS_ENT_ID(e)), comp_type, 0);

}

void recs_entity_remove_tag(struct recs *ecs, recs_entity e, recs_tag tag) {
  bitmask_set(bitmask_list_get(&ecs->comp_bitmask_list, RECS_ENT_ID(e)), recs_tag_id_to_comp_id(ecs, tag), 0);
}

void recs_entity_remove_all_components(struct recs *ecs, recs_entity e) {

  //remove components from component arrays
  for(recs_component t = 0; t < ecs->max_registered_components; t++) {
    if(bitmask_test(bitmask_list_get(&ecs->comp_bitmask_list, RECS_ENT_ID(e)), t)) {
      recs_entity_remove_component(ecs, e, t);

    }
  }

  //mark entity as having no components to clear tags
  bitmask_clear(bitmask_list_get(&ecs->comp_bitmask_list, e), 0, ecs->comp_bitmask_size);

}

int recs_entity_has_component(struct recs *ecs, recs_entity e, recs_component c) {
  return bitmask_test(bitmask_list_get(&ecs->comp_bitmask_list, RECS_ENT_ID(e)), c);
}

int recs_entity_has_tag(struct recs *ecs, recs_entity e, recs_tag tag) {
  return bitmask_test(bitmask_list_get(&ecs->comp_bitmask_list, RECS_ENT_ID(e)), recs_tag_id_to_comp_id(ecs, tag));
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



int recs_entity_matches_component_mask(struct recs *ecs, recs_entity e, uint8_t *mask, enum recs_ent_match_op match_op) {

  uint8_t *mask_for_entity = bitmask_list_get(&ecs->comp_bitmask_list, RECS_ENT_ID(e));

  //check all bytes except last one
  for(uint32_t i = 0; i < ecs->comp_bitmask_size - 1; i++) {
    uint8_t res = mask_for_entity[i] & mask[i];
    switch(match_op) {
      case RECS_ENT_MATCH_ALL: {
        if(res != mask[i]) {
          return 0;
        }
        break;
      }
      case RECS_ENT_MATCH_ANY: {
        if(res != 0) {
          return 1;
        }
        break;
      }
    }
    
  }

  //only check the LSB on the last byte

  //get number of bits (starting from MSB) that we are setting to 0 so that we can check the equality of the LSB bits.
  
  //do not account for when mask_bits == 0, as this should not be possible
  //when mask_bits = 1, num_unused_bits = 7
  //when mask_bits = 2, num_unused_bits = 6
  //when mask_bits = 3, num_unused_bits = 5
  //when mask_bits = 4, num_unused_bits = 4
  //when mask_bits = 7, num_unused_bits = 1
  //when mask_bits = 8, num_unused_bits = 0
  //when mask_bits = 9, num_unused_bits = 7
  
  // if mask_bits % 8 != 0
    //num_unused_bits == 8 - (mask_bits % 8) == 8 - (mask_bits & 7)
  // else
  // num_unused_bits = 0
  
  const uint32_t mask_bits = ecs->max_registered_components + ecs->max_tags;
  const uint8_t num_unused_bits = (mask_bits & 7) == 0 ? 0 : 8 - (mask_bits & 7);

  






  //grab the last byte and AND with (0xFF >> num_unused_bits) to set all unused MSB bits to 0
  uint8_t last_byte1 = mask_for_entity[ecs->comp_bitmask_size-1] & ((uint8_t)0xFF >> num_unused_bits);
  uint8_t last_byte2 = mask[ecs->comp_bitmask_size-1] & ((uint8_t)0xFF >> num_unused_bits);

  switch(match_op) {
    case RECS_ENT_MATCH_ALL: {
      return (last_byte1 & last_byte2) == last_byte2;
    }
    case RECS_ENT_MATCH_ANY: {
      return (last_byte1 & last_byte2) != 0;
    }

  }
  return 0;
}



int recs_entity_has_components(struct recs *ecs, recs_entity e, uint8_t *mask) {
  return recs_entity_matches_component_mask(ecs, e, mask, RECS_ENT_MATCH_ALL);
}


int recs_entity_has_excluded_components(struct recs *ecs, recs_entity e, uint8_t *mask) {
  return !recs_entity_matches_component_mask(ecs, e, mask, RECS_ENT_MATCH_ANY);
}

uint8_t recs_entity_active(struct recs *ecs, recs_entity e) {
  return !RECS_ENTITY_NONE(e) && RECS_ENT_VERSION(e) == ecs->ent_man.ent_versions_list[RECS_ENT_ID(e)];
}


void recs_bitmask_create(struct recs *ecs, uint8_t *mask, const uint32_t num_comps, const recs_component *comps, const uint32_t num_tags, const recs_tag *tags) {
  bitmask_clear(mask, 0, ecs->comp_bitmask_size);
  for(uint32_t i = 0; i < num_comps; i++) {
    bitmask_set(mask, comps[i], 1);
  }
  for(uint32_t i = 0; i < num_tags; i++) {
    bitmask_set(mask, recs_tag_id_to_comp_id(ecs, tags[i]), 1);
  }

}


static recs_entity recs_ent_iter_find(struct recs *ecs, recs_ent_iter *iter) {
  //assert that at least one of the 2 bitmasks are non-null
  RECS_ASSERT(!(iter->include_bitmask == NULL && iter->exclude_bitmask == NULL));


  for(; iter->index < ecs->ent_man.num_active_entities; iter->index++) {
    recs_entity e = ecs->ent_man.entity_pool[iter->index];

    //skip over recently deleted entities that have not been removed from the
    //active pool yet.
    if(!recs_entity_active(ecs, e)) continue;

    //uint8_t has_comps =    iter->include_bitmask == NULL || (iter->include_bitmask != NULL && recs_entity_has_components(ecs, e, iter->include_bitmask));
    //uint8_t has_ex_comps = iter->exclude_bitmask == NULL || (iter->exclude_bitmask != NULL && recs_entity_has_excluded_components(ecs, e, iter->exclude_bitmask));

    uint8_t has_comps =    iter->include_bitmask == NULL || (iter->include_bitmask != NULL && recs_entity_matches_component_mask(ecs, e, iter->include_bitmask, iter->include_op));
    uint8_t has_ex_comps = iter->exclude_bitmask == NULL || (iter->exclude_bitmask != NULL && !recs_entity_matches_component_mask(ecs, e, iter->exclude_bitmask, iter->exclude_op));

    if(has_comps && has_ex_comps) {
      iter->index++;
      return e;
    }
  }
  
  return RECS_NO_ENTITY;
}

recs_ent_iter recs_ent_iter_init(struct recs *ecs, uint8_t *mask) {
  recs_ent_iter iter = {
    .next_entity = RECS_NO_ENTITY,
    .index = 0,
    .include_bitmask = mask,
    .include_op = RECS_ENT_MATCH_ALL,
    .exclude_bitmask = NULL,
    .exclude_op = RECS_ENT_MATCH_ANY
  };


  //we need to find the 1st element such that when we call next(), we can obtain the next element.
  iter.next_entity = recs_ent_iter_find(ecs, &iter);

  //state that this iterator has performed a search for the next entity
  //iter.checked_for_next = 1;

  return iter;
}

recs_ent_iter recs_ent_iter_init_with_match(struct recs *ecs, uint8_t *mask, enum recs_ent_match_op match_op) {
  recs_ent_iter iter = {
    .next_entity = RECS_NO_ENTITY,
    .index = 0,
    .include_op = match_op,
    .include_bitmask = mask,
    .exclude_op = RECS_ENT_MATCH_ANY,
    .exclude_bitmask = NULL,
  };


  //we need to find the 1st element such that when we call next(), we can obtain the next element.
  iter.next_entity = recs_ent_iter_find(ecs, &iter);

  //state that this iterator has performed a search for the next entity
  //iter.checked_for_next = 1;

  return iter;
}

//  If we want to exclude 0011 0100 
//  Mask is 0000 1011
//          0011 0100
// Just invert the mask:
//          1111 0100
//          0011 0100

recs_ent_iter recs_ent_iter_init_with_exclude(struct recs *ecs, uint8_t *include_mask, uint8_t *exclude_mask) {
  recs_ent_iter iter = {
    .next_entity = RECS_NO_ENTITY,
    .index = 0,
    .include_bitmask = include_mask,
    .include_op = RECS_ENT_MATCH_ALL,
    .exclude_bitmask = exclude_mask,
    .exclude_op = RECS_ENT_MATCH_ANY,

  };

  //we need to find the 1st element such that when we call next(), we can obtain the next element.
  //we need to find the 1st element such that when we call next(), we can obtain the next element.
  iter.next_entity = recs_ent_iter_find(ecs, &iter);

  //state that this iterator has performed a search for the next entity
  //iter.checked_for_next = 1;

  return iter;
}

recs_ent_iter recs_ent_iter_init_with_exclude_and_match_op(struct recs *ecs, uint8_t *include_mask, enum recs_ent_match_op include_match_op, uint8_t *exclude_mask, enum recs_ent_match_op exclude_match_op) {
   recs_ent_iter iter = {
    .next_entity = RECS_NO_ENTITY,
    .index = 0,
    .include_bitmask = include_mask,
    .include_op = include_match_op,
    .exclude_bitmask = exclude_mask,
    .exclude_op = exclude_match_op
  };

  //we need to find the 1st element such that when we call next(), we can obtain the next element.
  //we need to find the 1st element such that when we call next(), we can obtain the next element.
  iter.next_entity = recs_ent_iter_find(ecs, &iter);

  //state that this iterator has performed a search for the next entity
  //iter.checked_for_next = 1;

  return iter;
}


uint8_t recs_ent_iter_has_next(recs_ent_iter *iter) {
  return RECS_ENT_ID(iter->next_entity) != RECS_NO_ENTITY_ID;
}

recs_entity recs_ent_iter_next(struct recs *ecs, recs_ent_iter *iter) {
  //grab the precached next entity
  recs_entity rtn = iter->next_entity;
  
  //search for next entity so that the has_next function works correctly
  iter->next_entity = recs_ent_iter_find(ecs, iter);

  
  return rtn;
}
