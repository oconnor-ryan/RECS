
#include "entity_manager.h"
#include "bitmask.h"
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
  struct component_pool recs_component_stores[RECS_MAX_COMPONENTS];

  uint32_t num_registered_components;
  uint32_t num_registered_systems;
  uint32_t num_system_types;


  //used to know what components each entity has
  struct recs_comp_bitmask comp_bitmask_list[RECS_MAX_ENTITIES];

  //a user provided pointer that contains extra data about their app state that needs to be seen/modified by
  //an ECS system.
  void *system_context; 

  struct recs_system systems[RECS_MAX_SYSTEMS];
  struct system_group_mapper system_group_mappers[RECS_MAX_SYS_GROUPS];

  struct entity_manager ent_man;

};

//all tags appear AFTER the recs_components with data.
static inline uint32_t recs_tag_id_to_comp_id(recs_tag tag) {
  uint32_t id = tag + RECS_MAX_COMPONENTS;
  RECS_ASSERT(id < RECS_MAX_TAGS + RECS_MAX_COMPONENTS);
  return id;
}



uint32_t recs_num_active_entities(struct recs *recs) {
  return recs->ent_man.num_active_entities;
}



recs recs_init(void *context) {

  uint32_t max_comps = RECS_MAX_COMPONENTS + RECS_MAX_TAGS;

  //RECS_ASSERT that max_comps does not overflow
  RECS_ASSERT(max_comps > RECS_MAX_COMPONENTS && max_comps > RECS_MAX_TAGS);

  //RECS_ASSERT that we don't have 2^32 - 1 entities, since the largest 32-bit unsigned
  //integer is used as a marker for something with no entities
  RECS_ASSERT(RECS_MAX_ENTITIES != RECS_NO_ENTITY_ID);





  //get sizes needed for each buffer
  size_t recs_buffer_size = sizeof(struct recs);

  //allocate one big buffer that will store nearly all of the ECS's dynamically allocated data.
  uint8_t *big_buffer = (uint8_t*)RECS_MALLOC(recs_buffer_size);
  if(big_buffer == NULL) {
    return NULL;
  }

  recs ecs = (recs) big_buffer;


  //assign all buffers to appropriate parts of ECS.


  ecs->num_registered_components = 0;
  ecs->num_registered_systems = 0;
  ecs->system_context = context;

  entity_manager_init(&ecs->ent_man);


  for(uint32_t i = 0; i < RECS_MAX_ENTITIES; i++) {
    bitmask_clear(ecs->comp_bitmask_list + i, 0);
  }

  //initialize each mapper with 0 systems by default
  for(uint32_t i = 0; i < RECS_MAX_SYS_GROUPS; i++) {
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
  RECS_ASSERT(ecs->num_registered_components < RECS_MAX_COMPONENTS);

  //register component with attached data
  struct component_pool *cp = ecs->recs_component_stores + type;



  if(!component_pool_init(cp, comp_size, max_components)) {
    return 0;
  }

  ecs->num_registered_components++;

  return 1;
}




void recs_system_register(struct recs *ecs, recs_system_func func, recs_system_group group) {
  RECS_ASSERT(ecs->num_registered_systems < RECS_MAX_SYSTEMS);


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
    for(uint32_t i = 0; i < RECS_MAX_SYS_GROUPS; i++) {
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
  RECS_ASSERT(ecs->ent_man.num_active_entities < RECS_MAX_ENTITIES);

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
  bitmask_set(ecs->comp_bitmask_list + e, comp_type, 1);
}

void recs_entity_add_tag(struct recs *ecs, recs_entity e, recs_tag tag) {
  bitmask_set(ecs->comp_bitmask_list + e, recs_tag_id_to_comp_id(tag), 1);
}

void recs_entity_remove_component(struct recs *ecs, recs_entity e, recs_component comp_type) {
  struct component_pool *ca =  ecs->recs_component_stores + comp_type;
  component_pool_remove(ca, e);

  //clear bit
  bitmask_set(ecs->comp_bitmask_list + e, comp_type, 0);

}

void recs_entity_remove_tag(struct recs *ecs, recs_entity e, recs_tag tag) {
  bitmask_set(ecs->comp_bitmask_list + e, recs_tag_id_to_comp_id(tag), 0);
}

void recs_entity_remove_all_components(struct recs *ecs, recs_entity e) {

  //remove components from component arrays
  for(recs_component t = 0; t < RECS_MAX_COMPONENTS; t++) {
    if(bitmask_test(ecs->comp_bitmask_list + e, t)) {
      recs_entity_remove_component(ecs, e, t);

    }
  }

  //mark entity as having no components to clear tags
  bitmask_clear(ecs->comp_bitmask_list + e, 0);

}

int recs_entity_has_component(struct recs *ecs, recs_entity e, recs_component c) {
  return bitmask_test(ecs->comp_bitmask_list + e, c);
}

int recs_entity_has_tag(struct recs *ecs, recs_entity e, recs_tag tag) {
  return bitmask_test(ecs->comp_bitmask_list + e, recs_tag_id_to_comp_id(tag));
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

int recs_entity_has_components(struct recs *recs, recs_entity e, struct recs_comp_bitmask mask) {
  struct recs_comp_bitmask res;
  bitmask_and(&res, recs->comp_bitmask_list + e, &mask);

  return bitmask_eq(&res, &mask);
}

int recs_entity_has_tags(struct recs *recs, recs_entity e, uint32_t num_tags, recs_tag *t) {
  for(uint32_t i = 0; i < num_tags; i++) {
    if(!recs_entity_has_tag(recs, e, t[i])) {
      return 0;
    }
  }
  return 1;
}

recs_comp_bitmask recs_bitmask_create(const uint32_t num_comps, const recs_component *comps, const uint32_t num_tags, const recs_tag *tags) {
  struct recs_comp_bitmask m;
  bitmask_clear(&m, 0);
  for(uint32_t i = 0; i < num_comps; i++) {
    bitmask_set(&m, comps[i], 1);
  }
  for(uint32_t i = 0; i < num_tags; i++) {
    bitmask_set(&m, recs_tag_id_to_comp_id(tags[i]), 1);
  }

  return m;
}


recs_ent_iter recs_ent_iter_init(recs_comp_bitmask mask) {
  return (recs_ent_iter) {
    .current_entity = RECS_NO_ENTITY_ID,
    .index = 0,
    .mask = mask
  };
}

uint8_t recs_ent_iter_has_next(struct recs *ecs, recs_ent_iter *iter) {
  return iter->index < ecs->ent_man.num_active_entities;
}

uint8_t recs_ent_iter_next(struct recs *ecs, recs_ent_iter *iter) {

  for(; iter->index < ecs->ent_man.num_active_entities; iter->index++) {
    recs_entity e = ecs->ent_man.set_of_ids[iter->index];
    if(recs_entity_has_components(ecs, e, iter->mask)) {
      iter->current_entity = e;
      iter->index++;
      return 1;
    }
  }
  iter->current_entity = RECS_NO_ENTITY_ID;
  return 0;
}
