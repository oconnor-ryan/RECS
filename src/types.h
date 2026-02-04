#ifndef TYPES_H
#define TYPES_H

// This file keeps the internal types used for this library.

#include "recs.h"


/*
  Bitmask Stuff
*/

#define BYTE_INDEX(bit_index) ((bit_index) >> 3)


#define NO_COMP_ID RECS_NO_ENTITY_ID


/* 
  Component Pool:
  Stores the raw data of every entity's components and maps entity IDs to components within its raw buffer.
*/

struct component_pool {
  char *buffer;
  uint32_t component_size;

  uint32_t num_components;
  uint32_t max_components;
  uint32_t max_entities;

  //rather than implementing a hash map, we will use 
  //arrays to map entity IDs to component indexes.
  uint32_t *entity_to_comp;

  //this is needed since when adding/removing components, we keep 
  //component data contiguous by moving the last component into the component
  //being removed. This requires us to keep track of each component->entity mapping
  //so that we can properly update our entity->comp mapping.
  uint32_t *comp_to_entity;

};



/*
  In order to make it possible to add and remove entities during iteration, 
  we could use the following method:

  - If we store our active IDs as a list of recs_entity rather than just the ID, we could just skip over
  recently deleted active IDs during iteration, then call a function to permanently free those IDs to the inactive ID pool.
  As for inactive ids, we can "cheat" by giving them an unused version number. This way, 
  both the active and inactive ids are the same size and we can still use our method of filling
  holes left behind by removed ids.
*/
/*
  We first initialize a set with the IDs 1 through MAX_ENTITIES.
  From this point on, we cannot allow duplicates within this set, 
  so we need our public header functions to avoid allowing users to insert duplicates.

  In the below state, no entities are active
  V 
  1, 2, 3, 4

  In the below state, entities 1 and 2 are active
        V
  1, 2, 3, 4

  After removing entity 1, we swap last active element with removed element and decrement end pointer
     V
  2, 1, 3, 4

  When adding another entity, simply increment the end pointer:

        V
  2, 1, 3, 4

  When remove entity 1 again:

     V  
  2, 3, 1, 4

  This way, we don't need 2 stacks of the same size to store active/inactive 
  IDs.

  We just read the left side for active IDs, and the right side for inactive entities.


*/
/*
  Entity Manager:

  This datatype handles the active Entity ID pool, returning IDs that are available for use as well as making IDs that
  were deleted ready for reuse.
*/
struct entity_manager {
  //stores list of all unused entity IDs.
  recs_entity *entity_pool;
  
  //store current version numbers for all entity IDs
  uint32_t *ent_versions_list;

  uint32_t num_active_entities;
  uint32_t max_entities;
};


struct recs_system {
  recs_system_func func;
};

struct bitmask_list {
  uint32_t bytes_per_mask;
  uint8_t *buffer;
};

//marks where groups of systems start at within our systems buffer.
struct system_group_mapper {
  uint32_t num_systems;
  uint32_t starting_index;
};

struct recs {
  struct component_pool *recs_component_stores;

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


#endif// TYPES_H
