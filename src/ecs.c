#include "bitmask.h"
#include "types.h"
#include "tags.h"







uint32_t recs_num_active_entities(struct recs *recs) {
  return recs->ent_man.num_active_entities;
}




recs recs_copy(recs og) {


  //since the entire ECS lies inside a single contiguous block of memory,
  //all we have to do is:
  //1. Allocate another buffer with the same size
  //2. Copy the contents of the old buffer to the new one
  //3. Update all the pointers to point to addresses that lie inside the new buffer

  //The 3rd step is easier said than done though, since we need to set up this buffer
  //the EXACT same way as we do in recs_init()


  size_t bytes_per_bitmask = RECS_GET_BITMASK_SIZE(og->max_registered_components, og->max_tags);

  //perform shallow copy of original RECS to new instance.
  struct recs ecs_static = *og;

  //we still need to allocate the new ECS as well as update the pointers to point to the newly allocated 
  //buffer


  //get sizes for each dynamically sized buffer in the RECS
  size_t recs_buffer_size = sizeof(struct recs);
  size_t entity_id_buffer_size = sizeof(recs_entity) * ecs_static.ent_man.max_entities;
  size_t entity_version_buffer_size = sizeof(uint32_t) * ecs_static.ent_man.max_entities;
  size_t bitmask_buffer_size = bytes_per_bitmask * ecs_static.ent_man.max_entities;
  size_t system_buffer_size = sizeof(struct recs_system) * ecs_static.max_registered_systems;
  size_t system_mapper_buffer_size = sizeof(struct system_group_mapper) * ecs_static.max_system_groups;
  size_t component_pool_buffer_size = sizeof(struct component_pool) * ecs_static.max_registered_components;


  //get size for each component_pool's buffer
  size_t final_size = 0;
  final_size += recs_buffer_size + entity_id_buffer_size + entity_version_buffer_size + component_pool_buffer_size;
  final_size += bitmask_buffer_size + system_buffer_size + system_mapper_buffer_size;

  size_t component_pool_inner_buffer_size = 0;
  for(uint32_t i = 0; i < ecs_static.max_registered_components; i++) {

    size_t comp_buffer_size = og->recs_component_stores[i].component_size * og->recs_component_stores[i].max_components;
    size_t ent_to_comp_buffer_size = sizeof(uint32_t) * og->ent_man.max_entities;
    size_t comp_to_ent_buffer_size = sizeof(uint32_t) * og->recs_component_stores[i].max_components;

    component_pool_inner_buffer_size += comp_buffer_size + ent_to_comp_buffer_size + comp_to_ent_buffer_size;
  }

  final_size += component_pool_inner_buffer_size;



  //allocate one big buffer that will store ALL of the ECS data
  uint8_t *big_buffer = (uint8_t*)RECS_MALLOC(final_size);
  if(big_buffer == NULL) {
    return NULL;
  }

  //set the returned RECS instance to the start of the buffer
  recs ecs = (recs) big_buffer;

  //copy static shallow copy of ecs to our newly allocated ecs
  *ecs = ecs_static;

  //update pointers in entity manager
  uint8_t *entity_id_buffer =      big_buffer + recs_buffer_size;
  uint8_t *entity_version_buffer = entity_id_buffer + entity_id_buffer_size;
  ecs->ent_man.entity_pool = (recs_entity*)entity_id_buffer;
  ecs->ent_man.ent_versions_list = (uint32_t*)entity_version_buffer;

  //copy values from old entity manager to the new one
  memcpy(ecs->ent_man.entity_pool, og->ent_man.entity_pool, entity_id_buffer_size);
  memcpy(ecs->ent_man.ent_versions_list, og->ent_man.ent_versions_list, entity_version_buffer_size);

  //update pointers in entity-component bitmask list
  uint8_t *bitmask_buffer =        entity_version_buffer + entity_version_buffer_size;
  ecs->comp_bitmask_list.buffer = bitmask_buffer;

  //copy from old bitmask list to new one
  memcpy(ecs->comp_bitmask_list.buffer, og->comp_bitmask_list.buffer, bitmask_buffer_size);

  //update pointers to systems and system_mappers
  uint8_t *system_buffer =         bitmask_buffer + bitmask_buffer_size;
  uint8_t *system_mapper_buffer =  system_buffer + system_buffer_size;
  ecs->systems = (struct recs_system*)system_buffer;
  ecs->system_group_mappers = (struct system_group_mapper*)system_mapper_buffer;

  //copy systems and mappers from old list to new one
  memcpy(ecs->systems, og->systems, system_buffer_size);
  memcpy(ecs->system_group_mappers, og->system_group_mappers, system_mapper_buffer_size);

  //update the pointer to the component pool list
  uint8_t *component_pool_buffer = system_mapper_buffer + system_mapper_buffer_size;
  ecs->recs_component_stores = (struct component_pool*) component_pool_buffer;
  
  //copy the values from each of the old component pools to their respective new ones
  uint8_t *next_buffer = component_pool_buffer + component_pool_buffer_size;
  for(uint32_t i = 0; i < ecs->max_registered_components; i++) {
    
    size_t comp_buffer_size = og->recs_component_stores[i].component_size * og->recs_component_stores[i].max_components;
    size_t ent_to_comp_buffer_size = sizeof(uint32_t) * og->ent_man.max_entities;
    size_t comp_to_ent_buffer_size = sizeof(uint32_t) * og->recs_component_stores[i].max_components;

    uint8_t *comp_buffer = next_buffer;
    uint8_t *ent_to_comp_buffer = comp_buffer + comp_buffer_size;
    uint8_t *comp_to_ent_buffer = ent_to_comp_buffer + ent_to_comp_buffer_size;

    //update the values and pointers in the component pool
    ecs->recs_component_stores[i].num_components = og->recs_component_stores[i].num_components;
    ecs->recs_component_stores[i].max_components = og->recs_component_stores[i].max_components;
    ecs->recs_component_stores[i].max_entities = og->recs_component_stores[i].max_entities;
    ecs->recs_component_stores[i].component_size = og->recs_component_stores[i].component_size;
    ecs->recs_component_stores[i].buffer = (char*) comp_buffer;
    ecs->recs_component_stores[i].entity_to_comp = (uint32_t*)ent_to_comp_buffer;
    ecs->recs_component_stores[i].comp_to_entity = (uint32_t*)comp_to_ent_buffer;

    size_t buffer_size = comp_buffer_size + ent_to_comp_buffer_size + comp_to_ent_buffer_size;
    //copy buffer from old component pool to new one
    memcpy(ecs->recs_component_stores[i].buffer, og->recs_component_stores[i].buffer, buffer_size);

    next_buffer += buffer_size;
  }


  return ecs;
}

void recs_free(struct recs *ecs) {
  if(ecs == NULL) {
    return;
  }

  //remember that we made 1 BIG allocation to store all data, starting at where the struct recs is at
  RECS_FREE(ecs);
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
