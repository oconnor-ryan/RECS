#ifndef RECS_H
#define RECS_H


#include <stdint.h>
#include <stddef.h>

//can define you own version of malloc(size_t size) from stdlib.h
#ifndef RECS_MALLOC
  #include <stdlib.h>
  #define RECS_MALLOC(size) malloc(size)
#endif

//can define you own version of free(void *ptr) from stdlib.h
#ifndef RECS_FREE
  #include <stdlib.h>
  #define RECS_FREE(ptr) free(ptr)
#endif

//can define you own version of assert(int boolean) from assert.h
#ifndef RECS_ASSERT
  #include <assert.h>
  #define RECS_ASSERT(boolean) assert(boolean)
#endif


// An invalid entity ID. This macro is used for variables of type
// uint32_t when you want to specify that there is no entity.
#define RECS_NO_ENTITY_ID 0xFFFFFFFF




//get the size (in bytes) of the bitmask being used to check tags and components.
#define RECS_GET_BITMASK_SIZE(max_components, max_tags) (((max_components) + (max_tags) + 8 - 1) / 8)

#define RECS_ENT_FROM(id, version) ((recs_entity) (((uint64_t)(id)) | ((uint64_t)(version) << 32)))
#define RECS_ENT_VERSION(ent) ((uint32_t)((ent) >> 32))
#define RECS_ENT_ID(ent) ((uint32_t)(ent))

#define RECS_NO_ENTITY (RECS_ENT_FROM(RECS_NO_ENTITY_ID, 0))



//used to check if the recs_entity contains the RECS_NO_ENTITY_ID (indicating a non-existant id)
#define RECS_ENTITY_NONE(ent) (RECS_ENT_ID(ent) == RECS_NO_ENTITY_ID)


// logical operators to use when iterating over entities in recs_ent_iter
enum recs_ent_match_op {
  RECS_ENT_MATCH_ALL, //make sure entity has a specific relation with ALL listed components
  RECS_ENT_MATCH_ANY, //make sure entity has a specific relation with ANY listed components

};

typedef uint32_t recs_component;
typedef uint64_t recs_entity;
typedef uint32_t recs_tag;
typedef uint32_t recs_system_group;



typedef struct recs_entity_iterator {
  recs_entity next_entity;
  uint32_t index;
  uint8_t *include_bitmask;

  // default operation is an ALL
  enum recs_ent_match_op include_op;

  uint8_t *exclude_bitmask;

  // default operation is an ANY 
  enum recs_ent_match_op exclude_op;



  //the maximum index in the active entity list to search though.
  //If 0, this index is ignored and the iterator will continue iterating
  //even if the active entity list grows.
  uint32_t max_entity_index;


} recs_ent_iter;


typedef struct recs *recs;


typedef void (*recs_system_func)(struct recs *ecs);

//configuration struct

struct recs_init_config_component {
    recs_component type;
    size_t comp_size;
    uint32_t max_components;
};

struct recs_init_config_system {
  recs_system_func func;
  recs_system_group group;
};


struct recs_init_config {
  uint32_t max_entities;
  uint32_t max_component_types;
  uint32_t max_tags;
  uint32_t max_systems;
  uint32_t max_system_groups;
  void *context;

  struct recs_init_config_component *components;
  struct recs_init_config_system *systems;

  

};

//convienence macros for initializing enums for component ids, tags, and system groups.
//this reduces the chance of a user making the mistake of defining custom values in their enum definition,
//which would prevent the ECS from working correctly.
// This is due to each enum needing to be defined such that each enum value is defined as 0, 1, 2, 3, ...,
// since these are indices to arrays.

#define RECS_INIT_COMP_IDS(comp_enum_name, ...) enum comp_enum_name { __VA_ARGS__ }
#define RECS_INIT_TAG_IDS(tag_enum_name, ...) enum tag_enum_name { __VA_ARGS__ }
#define RECS_INIT_SYS_GRP_IDS(sys_enum_name, ...) enum sys_enum_name { __VA_ARGS__ }



//used specifically in the recs_create_bitmask() function to reduce boilerplate code 
//for specifying what tags and components you want your bitmask to check for.
#define RECS_BITMASK_CREATE_COMP_ARG(num_comps, ...) num_comps, (recs_component[]){__VA_ARGS__}
#define RECS_BITMASK_CREATE_TAG_ARG(num_tags, ...) num_tags, (recs_tag[]){__VA_ARGS__}

#define RECS_BITMASK_CREATE(var, bitmask_size, comp_arg, tag_arg) \
  uint8_t buffer[bitmask_size]; \
  (recs_comp_bitmask)buffer


//allocate and initialize a RECS instance. Returns NULL if initialization failed.
recs recs_init(struct recs_init_config config);

//performs a deep copy of the ECS and returns a pointer to the new copy, or NULL if it fails.
recs recs_copy(recs ecs);

//free RECS instance from memory, destroying all entities, components, and systems
void recs_free(struct recs *recs);


//get a component directly from the component pool's raw buffer.
void* recs_component_get(struct recs *recs, recs_component c, uint32_t index);

//get the number of active instances of a component
uint32_t recs_component_num_instances(struct recs *recs, recs_component c);

//get the entity associated with the component at the component index to the raw component buffer.
recs_entity recs_component_get_entity(struct recs *recs, recs_component c, uint32_t comp_index);




//set user-defined context that allows systems to interact with external 
//data
void recs_system_set_context(struct recs *recs, void *context);

//retrieve user-defined context. Can be called within systems to view
//and interact with external data.
void* recs_system_get_context(struct recs *recs);

//run a set of systems within a system group. Each system executes in the 
//same order as the order were registered in.
void recs_system_run(struct recs *recs, recs_system_group group);



//check the number of active entities
uint32_t recs_num_active_entities(struct recs *recs);


//add an entity without components
recs_entity recs_entity_add(struct recs *recs);

/* TODO: Allow the following to be done
  1. Immediately remove entity from being processed in a system
  2. Immediately add entity that can be processed as soon as it spawns
  3. Queue an entity to be removed, but still allow it to be processed while finishing iteration.
  4. Queue an entity to be added, but not allowing it to process until iteration is done.

  We currently only natively support options 1 and 2. While users of this library can 
  technically achieve options 3 and 4 using tags and a custom system, I would like this
  to not be necessary.

  This could be accomplished by adding booleans to the entity iterators, which can choose
  to skip entities being added or locking the "num_entities" limit when queuing newly added entities.
*/

//queue an entity to be removed and disable it from being found in recs_ent_iter iterators.
void recs_entity_queue_remove(struct recs *ecs, recs_entity e);

//remove all entities queued for removal from the active entity pool, 
//freeing their IDs to be reused
//NOTE: This should only be called when NOT ITERATING OVER ENTITIES using recs_ent_iter.
//If calling this while iterating, entities may be skipped.
void recs_entity_remove_queued(struct recs *ecs);

//immedately remove a single entity from the active entity pool.
//NOTE: This should only be called when NOT ITERATING OVER ENTITIES using recs_ent_iter.
//If calling this while iterating, entities may be skipped.
void recs_entity_remove(struct recs *ecs, recs_entity e);


//add a component to a specific entity.
void recs_entity_add_component(struct recs *recs, recs_entity e, recs_component comp_type, void *component);

//add a tag to a specific entity.
void recs_entity_add_tag(struct recs *recs, recs_entity e, recs_tag tag);


//remove a component from a specific entity
void recs_entity_remove_component(struct recs *recs, recs_entity e, recs_component comp_type);

//remove a tag from a specific entity
void recs_entity_remove_tag(struct recs *recs, recs_entity e, recs_tag tag);

//remove every component from a specific entity
void recs_entity_remove_all_components(struct recs *recs, recs_entity e);

//check if an entity has a specific component
int recs_entity_has_component(struct recs *recs, recs_entity e, recs_component c);

//check if an entity has a specific tag
int recs_entity_has_tag(struct recs *recs, recs_entity e, recs_tag tag);


//check if an entity has a set of components and tags specified in the bitmask provided.
int recs_entity_has_components(struct recs *recs, recs_entity e, uint8_t *mask);

//check if an entity does NOT HAVE ANY of the components and tags specified in the bitmask.
int recs_entity_has_excluded_components(struct recs *ecs, recs_entity e, uint8_t *mask);

// check if entity's component mask matches the provided mask based on the matching operator
int recs_entity_matches_component_mask(struct recs *ecs, recs_entity e, uint8_t *mask, enum recs_ent_match_op match_op);

//retrieve the component of a specific entity.
void* recs_entity_get_component(struct recs *recs, recs_entity e, recs_component c);

//check if an entity is active, or has been removed
uint8_t recs_entity_active(struct recs *ecs, recs_entity e);

//initialize a bitmask using an array of component IDs and an array of tag IDs.
void recs_bitmask_create(struct recs *recs, uint8_t *mask, const uint32_t num_comps, const recs_component *comps, const uint32_t num_tags, const recs_tag *tags);


//functions for entity iterator

//initialize an iterator to go through the list of active entities.
recs_ent_iter recs_ent_iter_init(struct recs *ecs, uint8_t *mask);

recs_ent_iter recs_ent_iter_init_with_match(struct recs *ecs, uint8_t *mask, enum recs_ent_match_op match_op);


recs_ent_iter recs_ent_iter_init_with_exclude(struct recs *ecs, uint8_t *include_mask, uint8_t *exclude_mask);

recs_ent_iter recs_ent_iter_init_with_exclude_and_match_op(struct recs *ecs, uint8_t *include_mask, enum recs_ent_match_op include_match_op, uint8_t *exclude_mask, enum recs_ent_match_op exclude_match_op);


//check if there are any more active entities left to process that have 
//the specified components and tags
uint8_t recs_ent_iter_has_next(recs_ent_iter *iter);

//retrieve the next entity ID and store it in the iterator. Returns 0
//if no entities are left to check. Returns 1 if an entity was found.
recs_entity recs_ent_iter_next(struct recs *ecs, recs_ent_iter *iter);



#endif


