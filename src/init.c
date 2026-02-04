#include "types.h"
#include "bitmask.h"

static inline void recs_system_register(struct recs *ecs, recs_system_func func, recs_system_group group) {
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


static inline void component_pool_init(struct component_pool *ca, unsigned char *buffer, uint32_t component_size, uint32_t max_components, uint32_t max_entities) {
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

static inline void entity_manager_init(struct entity_manager *em, uint8_t *id_buffer, uint8_t *version_buffer, uint32_t max_entities) {
  em->num_active_entities = 0;
  em->max_entities = max_entities;
  em->entity_pool = (recs_entity*)id_buffer;
  em->ent_versions_list = (uint32_t*) version_buffer;

  for(uint32_t i = 0; i < em->max_entities; i++) {
    //add initial entity IDs to set. 
    em->entity_pool[i] = RECS_ENT_FROM(i, 0); //note that version number is unused here, so any value is valid

    //set all versions to 0
    em->ent_versions_list[i] = 0;
  }

  

}

static inline void bitmask_list_init(struct bitmask_list *list, uint32_t bytes_per_mask, uint8_t *buffer) {
  list->bytes_per_mask = bytes_per_mask;
  list->buffer = buffer;
}



// Initialize the ECS.
// Note that you must provide all of the component types and systems you will use for this ECS into the configuration
// struct.
recs recs_init(const struct recs_init_config config) {


  //RECS_ASSERT that max_comps does not overflow
  RECS_ASSERT((config.max_component_types + config.max_tags) > config.max_component_types && (config.max_component_types + config.max_tags) > config.max_tags);

  //RECS_ASSERT that we don't have 2^32 - 1 entities, since the largest 32-bit unsigned
  //integer is used as a marker for something with no entities
  RECS_ASSERT(config.max_entities-1 != RECS_NO_ENTITY_ID);

  size_t bytes_per_bitmask = RECS_GET_BITMASK_SIZE(config.max_component_types, config.max_tags);

  struct recs ecs_static = {
    .max_registered_components = config.max_component_types,
    .max_registered_systems = config.max_systems,
    .max_system_groups = config.max_system_groups,
    .max_tags = config.max_tags,
    .comp_bitmask_size = bytes_per_bitmask,
    .system_context = config.context,
    .num_registered_systems = 0,
    .ent_man = {
      .max_entities = config.max_entities,
      .num_active_entities = 0,
      .entity_pool = NULL,
      .ent_versions_list = NULL
    },
    .comp_bitmask_list = {
      .bytes_per_mask = bytes_per_bitmask,
      .buffer = NULL
    },
    .systems = NULL,
    .system_group_mappers = NULL,
    .recs_component_stores = NULL
  };



  //get sizes needed for each buffer needed in the RECS
  size_t recs_buffer_size = sizeof(struct recs);
  size_t entity_id_buffer_size = sizeof(recs_entity) * config.max_entities;
  size_t entity_version_buffer_size = sizeof(uint32_t) * config.max_entities;
  size_t bitmask_buffer_size = bytes_per_bitmask * config.max_entities;
  size_t system_buffer_size = sizeof(struct recs_system) * config.max_systems;
  size_t system_mapper_buffer_size = sizeof(struct system_group_mapper) * config.max_system_groups;
  size_t component_pool_buffer_size = sizeof(struct component_pool) * config.max_component_types;


  //get size for each component_pool's buffer
  size_t final_size = 0;
  final_size += recs_buffer_size + entity_id_buffer_size + entity_version_buffer_size + component_pool_buffer_size;
  final_size += bitmask_buffer_size + system_buffer_size + system_mapper_buffer_size;

  size_t component_pool_inner_buffer_size = 0;
  for(uint32_t i = 0; i < config.max_component_types; i++) {
    RECS_ASSERT(config.components[i].max_components <= config.max_entities);
    RECS_ASSERT(config.components[i].comp_size > 0);

    size_t comp_buffer_size = config.components[i].comp_size * config.components[i].max_components;
    size_t ent_to_comp_buffer_size = sizeof(uint32_t) * config.max_entities;
    size_t comp_to_ent_buffer_size = sizeof(uint32_t) * config.components[i].max_components;


    component_pool_inner_buffer_size += comp_buffer_size + ent_to_comp_buffer_size + comp_to_ent_buffer_size;
  }

  final_size += component_pool_inner_buffer_size;



  //allocate one big buffer that will store ALL of the ECS data
  uint8_t *big_buffer = (uint8_t*)RECS_MALLOC(final_size);
  if(big_buffer == NULL) {
    return NULL;
  }

  recs ecs = (recs) big_buffer;

  //copy static ecs to allocated ecs
  *ecs = ecs_static;

  //init the entity manager
  uint8_t *entity_id_buffer =      big_buffer + recs_buffer_size;
  uint8_t *entity_version_buffer = entity_id_buffer + entity_id_buffer_size;
  entity_manager_init(&ecs->ent_man, entity_id_buffer, entity_version_buffer, config.max_entities);

  //init the entity-component bitmask list
  uint8_t *bitmask_buffer =        entity_version_buffer + entity_version_buffer_size;
  bitmask_list_init(&ecs->comp_bitmask_list, ecs->comp_bitmask_size, bitmask_buffer);
  for(uint32_t i = 0; i < config.max_entities; i++) {
    uint8_t *mask = bitmask_list_get(&ecs->comp_bitmask_list, i);
    bitmask_clear(mask, 0, bytes_per_bitmask);
  }

  //init the system mappers and systems list
  uint8_t *system_buffer =         bitmask_buffer + bitmask_buffer_size;
  uint8_t *system_mapper_buffer =  system_buffer + system_buffer_size;
  ecs->systems = (struct recs_system*)system_buffer;
  ecs->system_group_mappers = (struct system_group_mapper*)system_mapper_buffer;
  //initialize each mapper with 0 systems by default
  for(uint32_t i = 0; i < config.max_system_groups; i++) {
    ecs->system_group_mappers[i].num_systems = 0;
  }
  //register each system
  for(uint32_t i = 0; i < config.max_systems; i++) {
    recs_system_register(ecs, config.systems[i].func, config.systems[i].group);
  }


  //init the component pool list
  uint8_t *component_pool_buffer = system_mapper_buffer + system_mapper_buffer_size;
  ecs->recs_component_stores = (struct component_pool*) component_pool_buffer;
  

  //set up the buffer for each component pool
  unsigned char *next_buffer = component_pool_buffer + component_pool_buffer_size;
  for(uint32_t i = 0; i < config.max_component_types; i++) {
    RECS_ASSERT(config.components[i].comp_size > 0);
    
    size_t comp_buffer_size = config.components[i].comp_size * config.components[i].max_components;
    size_t ent_to_comp_buffer_size = sizeof(uint32_t) * config.max_entities;
    //only allocate to max_components since that is usually equal to 
    //or less than the max_entities, making memory storage slightly more efficient.
    size_t comp_to_ent_buffer_size = sizeof(uint32_t) * config.components[i].max_components;


    component_pool_init(
      ecs->recs_component_stores + config.components[i].type, 
      next_buffer, 
      config.components[i].comp_size, 
      config.components[i].max_components,
      config.max_entities
    );

    next_buffer += comp_buffer_size + ent_to_comp_buffer_size + comp_to_ent_buffer_size;
  }


  return ecs;
  
}
