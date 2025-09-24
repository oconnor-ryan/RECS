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

// An invalid entity ID. This value is used for variables of type
// recs_entity when you want to specify that there is no entity.
//TODO: Consider changing this to 0 for easier boolean checking
#define RECS_NO_ENTITY_ID 0xFFFFFFFF



//get the size (in bytes) of the bitmask being used to check tags and components.
#define RECS_GET_BITMASK_SIZE(max_components, max_tags) (((max_components) + (max_tags) + 8 - 1) / 8)

typedef uint32_t recs_component;
typedef uint32_t recs_entity;
typedef uint32_t recs_tag;
typedef uint32_t recs_system_group;



typedef struct recs_entity_iterator {
  recs_entity current_entity;
  uint32_t index;
  uint8_t *include_bitmask;
  uint8_t *exclude_bitmask;

} recs_ent_iter;


typedef struct recs *recs;


typedef void (*recs_system_func)(struct recs *ecs);

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
recs recs_init(uint32_t max_entities, uint32_t max_components, uint32_t max_tags, uint32_t max_systems, uint32_t max_sys_groups, void *context);

//free RECS instance from memory, destroying all entities, components, and systems
void recs_free(struct recs *recs);


//register a component for entities to use.
int recs_component_register(struct recs *recs, recs_component type, uint32_t max_instances, size_t comp_size);

//unregisters a component by removing this component from all entities and deleting the 
//component pool.
void recs_component_unregister(struct recs *ecs, recs_component type);

//get a component directly from the component pool's raw buffer.
void* recs_component_get(struct recs *recs, recs_component c, uint32_t index);


//register a system under a specific system group
void recs_system_register(struct recs *recs, recs_system_func func, recs_system_group type);



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

//remove an entity, deleting any components it had
void recs_entity_remove(struct recs *recs, recs_entity e);

//remove an entity using an index into the active entity list.
void recs_entity_remove_at_id_index(struct recs *recs, uint32_t id_index);

//grab an entity from the active entity list.
recs_entity recs_entity_get(struct recs *recs, uint32_t index);


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


//retrieve the component of a specific entity.
void* recs_entity_get_component(struct recs *recs, recs_entity e, recs_component c);


//initialize a bitmask using an array of component IDs and an array of tag IDs.
void recs_bitmask_create(struct recs *recs, uint8_t *mask, const uint32_t num_comps, const recs_component *comps, const uint32_t num_tags, const recs_tag *tags);


//functions for entity iterator

//initialize an iterator to go through the list of active entities.
recs_ent_iter recs_ent_iter_init(uint8_t *mask);

recs_ent_iter recs_ent_iter_init_with_exclude(uint8_t *include_mask, uint8_t *exclude_mask);


//check if there are any more active entities left to process that have 
//the specified components and tags
uint8_t recs_ent_iter_has_next(struct recs *ecs, recs_ent_iter *iter);

//retrieve the next entity ID and store it in the iterator. Returns 0
//if no entities are left to check. Returns 1 if an entity was found.
uint8_t recs_ent_iter_next(struct recs *ecs, recs_ent_iter *iter);



#endif


