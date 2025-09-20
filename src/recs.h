#ifndef RECS_H
#define RECS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifndef RECS_MALLOC
  #include <stdlib.h>
  #define RECS_MALLOC(size) malloc(size)
#endif

#ifndef RECS_FREE
  #include <stdlib.h>
  #define RECS_FREE(ptr) free(ptr)
#endif

#ifndef RECS_ASSERT
  #include <assert.h>
  #define RECS_ASSERT(boolean) assert(boolean)
#endif

#define RECS_NO_ENTITY_ID 0xFFFFFFFF

#ifndef RECS_MAX_ENTITIES
#define RECS_MAX_ENTITIES 1024
#endif


#ifndef RECS_MAX_COMPONENTS
#define RECS_MAX_COMPONENTS 64
#endif 

#ifndef RECS_MAX_TAGS
#define RECS_MAX_TAGS 64
#endif 

#ifndef RECS_MAX_SYSTEMS
#define RECS_MAX_SYSTEMS 64
#endif 

#ifndef RECS_MAX_SYS_GROUPS
#define RECS_MAX_SYS_GROUPS 16
#endif 

#define RECS_COMP_BITMASK_NUM_BITS (RECS_MAX_COMPONENTS + RECS_MAX_TAGS)
#define RECS_COMP_BITMASK_SIZE (1 + RECS_COMP_BITMASK_NUM_BITS / 8)

typedef uint32_t recs_component;
typedef uint32_t recs_entity;
typedef uint32_t recs_tag;
typedef uint32_t recs_system_group;

typedef struct recs_comp_bitmask {
  uint8_t bytes[RECS_COMP_BITMASK_SIZE];
} recs_comp_bitmask;


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


/*
  You must define the RECS_COMP_TO_ID_MAPPER macro if using RECS_MAP_COMP_TO_ID or RECS_MAP_COMP_PTR_TO_ID.

  The RECS_COMP_TO_ID_MAPPER must be written in a format similar to the _Generic keyword:

  #define RECS_COMP_TO_ID_MAPPER \
    <type>: <ENUM_OR_INTEGER_VALUE>, \
    <type>: <ENUM_OR_INTEGER_VALUE>, \
    <type>: <ENUM_OR_INTEGER_VALUE>, \
    ... (not part of macro definition, it just means that you can add as many <type>: <value> pairs as needed)
    <type>: <ENUM_OR_INTEGER_VALUE> 

  Here is an example of a definition for RECS_COMP_TO_ID_MAPPER
*/
/* 
#define RECS_COMP_TO_ID_MAPPER \
  struct componentA: COMP_ID_A_ENUM,
  struct componentB: COMP_ID_B_ENUM
*/


//using the type of the passed-in variable, grab the component ID associated with the variable's type
#define RECS_MAP_COMP_TO_ID(comp_variable) _Generic((comp_variable), \
  RECS_COMP_TO_ID_MAPPER)


//similar to RECS_MAP_COMP_TO_ID, except the comp_variable is a pointer to the type whose component ID you want.
#define RECS_MAP_COMP_PTR_TO_ID(comp_variable) _Generic((*comp_variable), \
  RECS_COMP_TO_ID_MAPPER)


#define RECS_ENTITY_ADD_COMP(ecs, entity, comp_var) recs_entity_add_component(ecs, entity, RECS_MAP_COMP_TO_ID(comp_var), &comp_var)


#define RECS_BITMASK_CREATE_COMP_ARG(num_comps, ...) num_comps, (recs_component[]){__VA_ARGS__}
#define RECS_BITMASK_CREATE_TAG_ARG(num_tags, ...) num_tags, (recs_tag[]){__VA_ARGS__}

#define RECS_TAG_TO_COMP(tag_id) (tag_id + RECS_MAX_COMPONENTS)


// a convienent variable for a zeroed-out bitmask
static const recs_comp_bitmask RECS_NONE_BITMASK = { .bytes = {0} };


recs recs_init(void *context);
void recs_free(struct recs *recs);

int recs_component_register(struct recs *recs, recs_component type, uint32_t max_instances, size_t comp_size);
void* recs_component_get(struct recs *recs, recs_component c, uint32_t index);



void recs_system_register(struct recs *recs, recs_system_func func, recs_system_group type);

void recs_system_set_context(struct recs *recs, void *context);
void* recs_system_get_context(struct recs *recs);

void recs_system_run(struct recs *recs, recs_system_group type);


uint32_t recs_num_active_entities(struct recs *recs);



recs_entity recs_entity_add(struct recs *recs);
void recs_entity_remove(struct recs *recs, recs_entity e);
void recs_entity_remove_at_id_index(struct recs *recs, uint32_t id_index);

recs_entity recs_entity_get(struct recs *recs, uint32_t index);


void recs_entity_add_component(struct recs *recs, recs_entity e, recs_component comp_type, void *component);
void recs_entity_add_tag(struct recs *recs, recs_entity e, recs_tag tag);

void recs_entity_remove_component(struct recs *recs, recs_entity e, recs_component comp_type);
void recs_entity_remove_tag(struct recs *recs, recs_entity e, recs_tag tag);
void recs_entity_remove_all_components(struct recs *recs, recs_entity e);

int recs_entity_has_component(struct recs *recs, recs_entity e, recs_component c);
int recs_entity_has_tag(struct recs *recs, recs_entity e, recs_tag tag);

int recs_entity_has_components(struct recs *recs, recs_entity e, struct recs_comp_bitmask mask);
int recs_entity_has_tags(struct recs *recs, recs_entity e, uint32_t num_tags, recs_tag *t);



void* recs_entity_get_component(struct recs *recs, recs_entity e, recs_component c);

recs_comp_bitmask recs_bitmask_create(const uint32_t num_comps, const recs_component *comps, const uint32_t num_tags, const recs_tag *tags);

//get the first entity starting at start_index that contains the components and tags listed in comp_ids and tags.
//This also updates start_index to the next index to iterate at when calling this in a loop

recs_entity recs_entity_get_next_with_comps(struct recs *ecs, recs_comp_bitmask mask, uint32_t *start_index);


#endif// RECS_H



// implementation section, you must define the RECS_IMPLEMENTATION macro BEFORE using #include "recs.h".
// This needs to be done in ONLY ONE .C file in your project. All other .C files should just 
// #include the recs.h header without the macro defined.
#ifdef RECS_IMPLEMENTATION

/*
  Entity Manager Section:

  This section handles the active Entity ID pool, returning IDs that are available for use as well as making IDs that
  were deleted ready for reuse.
*/

struct entity_manager {
  //stores list of all unused entity IDs.
  recs_entity set_of_ids[RECS_MAX_ENTITIES];
  uint32_t num_active_entities;
};

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

static void entity_manager_init(struct entity_manager *em) {
  em->num_active_entities = 0;

  //add initial entity IDs to set. 
  for(recs_entity i = 0; i < RECS_MAX_ENTITIES; i++) {
    em->set_of_ids[i] = i;
  }

}


static recs_entity entity_manager_add(struct entity_manager *em) {
  RECS_ASSERT(em->num_active_entities < RECS_MAX_ENTITIES);

  recs_entity e = em->set_of_ids[em->num_active_entities];
  em->num_active_entities++;

  return e;

}

static void entity_manager_remove_at_index(struct entity_manager *em, uint32_t active_entity_index) {
  uint32_t i = active_entity_index;

  //swap last ACTIVE ID with removed ID.

  uint32_t temp = em->set_of_ids[i];
  em->set_of_ids[i] = em->set_of_ids[em->num_active_entities-1];
  em->set_of_ids[em->num_active_entities-1] = temp;

  em->num_active_entities--;
}

static void entity_manager_remove(struct entity_manager *em, recs_entity e) {
  RECS_ASSERT(em->num_active_entities != 0);

  //this requires a search for the ID.
  for(uint32_t i = 0; i < em->num_active_entities; i++) {
    if(em->set_of_ids[i] == e) {
      //swap last ACTIVE ID with removed ID.
      entity_manager_remove_at_index(em, i);
      return;
    }
  }

}

/*
  Entity Manager End
*/



/*
  Bitmask Section.

  A utility for creating, comparing, and setting bitmasks used by each entity to know what components and tags
  are attached to that entity.
*/

#define BYTE_INDEX(bit_index) ((bit_index) >> 3)


static void bitmask_clear(struct recs_comp_bitmask *mask, uint8_t value) {
  memset(mask->bytes, value ? 0xFF : 0, RECS_COMP_BITMASK_SIZE);
}

static void bitmask_set(struct recs_comp_bitmask *mask, uint64_t bit_index, uint8_t value) {
  uint64_t byte_index = BYTE_INDEX(bit_index); //divide by 8

  //index & 0b111 == index % 8
  uint8_t m = (uint8_t)1 << (bit_index & 7);
  if(value) {
    mask->bytes[byte_index] |= m;
  } else {
    mask->bytes[byte_index] &= ~m;
  }
}

static uint8_t bitmask_test(struct recs_comp_bitmask *mask, uint64_t bit_index) {
  // example
  // there are 60 bytes, we want the 100th bit
  // 60 bytes = 480 bits
  // 100 / 8 = 12 
  // 12th byte = 96 bit
  // To get the bit within the 12th byte, we take the 
  // modulus 8 (or AND with 7).
  // 100 & 7 == 0110 0100 & 0000 0111 == 0000 0100 == 4

  uint64_t byte_index = BYTE_INDEX(bit_index); //divide by 8
  uint8_t byte = mask->bytes[byte_index];

  //get bit within this byte using modulus OR AND 0b111
  return byte & ((uint8_t)1 << (bit_index & 7));
}

static void bitmask_and(struct recs_comp_bitmask *dest, struct recs_comp_bitmask *op1, struct recs_comp_bitmask *op2) {
  for(uint32_t i = 0; i < RECS_COMP_BITMASK_SIZE; i++) {
    dest->bytes[i] = op1->bytes[i] & op2->bytes[i];
  }
}

static uint8_t bitmask_eq(struct recs_comp_bitmask *op1, struct recs_comp_bitmask *op2) {
  //the last byte will NOT use the full 8 bits if the RECS_MAX_COMPONENTS + RECS_MAX_TAGS are not a multiple of 8.
  //Thus we will need to set the MSB bits of the last byte to a consistant value (ususally 0).
  for(uint32_t i = 0; i < RECS_COMP_BITMASK_SIZE - 1; i++) {
    if(op1->bytes[i] != op2->bytes[i]) {
      return 0;
    }
  }

  //RECS_COMP_BITMASK_NUM_BITS & 7 == 0, then all bits are being checked
  //RECS_COMP_BITMASK_NUM_BITS & 7 == 1, then all but 1 bit is being checked

  //get number of bits (starting from MSB) that we are setting to 0 so that we can check the equality of the LSB bits.
  const uint8_t num_unused_bits = RECS_COMP_BITMASK_NUM_BITS & 7;

  //grab the last byte and AND with (0xFF >> num_unused_bits) to set all unused MSB bits to 0
  uint8_t last_byte1 = op1->bytes[RECS_COMP_BITMASK_SIZE-1] & ((uint8_t)0xFF >> num_unused_bits);
  uint8_t last_byte2 = op2->bytes[RECS_COMP_BITMASK_SIZE-1] & ((uint8_t)0xFF >> num_unused_bits);

  return last_byte1 == last_byte2;
}


/*
  Bitmask End
*/


/* 
  Component Pool Section

  Stores the raw data of every entity's components and maps entity IDs to components within its raw buffer.
  This also handles creating and freeing component pools.
*/

struct component_pool {
  char *buffer;
  uint32_t component_size;

  uint32_t num_components;
  uint32_t max_components;

  //rather than implementing a hash map, we will use 
  //arrays to map entity IDs to component indexes.
  uint32_t entity_to_comp[RECS_MAX_ENTITIES];

  //this is needed since when adding/removing components, we keep 
  //component data contiguous by moving the last component into the component
  //being removed. This requires us to keep track of each component->entity mapping
  //so that we can properly update our entity->comp mapping.
  recs_entity *comp_to_entity;

};

#define NO_COMP_ID RECS_NO_ENTITY_ID


static int component_pool_init(struct component_pool *ca, uint32_t component_size, uint32_t max_components) {
  ca->num_components = 0;
  ca->component_size = component_size;
  ca->max_components = max_components;

  //allocate buffer
  size_t comp_buffer_size = component_size * max_components;

  //only allocate to max_components since that is usually equal to 
  //or less than the max_entities, making memory storage slightly more efficient.
  size_t comp_to_ent_buffer_size = sizeof(recs_entity) * max_components;


  //allocate all memory at once needed to store raw component data,
  //entity to component mapper, and component to entity mapper.
  char *buffer = RECS_MALLOC(comp_buffer_size + comp_to_ent_buffer_size);
  if(buffer == NULL) {
    return 0;
  }
  char *comp_buffer = buffer;
  char *comp_to_ent_buffer = buffer + comp_buffer_size;

  ca->buffer = comp_buffer;
  ca->comp_to_entity = (uint32_t*)comp_to_ent_buffer;

  //mark all components as not belonging to any entity. 
  //Because this game will never get to a point where there are 65000 entities or
  //components, using a really big number as a marker for a non-existant component/entity
  //is perfectly find. 

  for(uint32_t i = 0; i < ca->max_components; i++) {
    ca->comp_to_entity[i] = RECS_NO_ENTITY_ID;
  }

  for(uint32_t i = 0; i < RECS_MAX_ENTITIES; i++) {
    ca->entity_to_comp[i] = NO_COMP_ID;
  }
  
  return 1;
}

static void *component_pool_get(struct component_pool *ca, recs_entity e) {
  
  uint32_t component_index = ca->entity_to_comp[e];

  if(component_index == NO_COMP_ID) {
    return NULL;
  }
  return ca->buffer + (ca->component_size * component_index);
}


static void component_pool_add(struct component_pool *ca, recs_entity e, void *component) {
  RECS_ASSERT(ca->num_components < ca->max_components);

  uint32_t component_index = ca->num_components;

  memcpy(ca->buffer + (ca->component_size * component_index), component, ca->component_size);

  ca->comp_to_entity[component_index] = e;
  ca->entity_to_comp[e] = component_index;

  ca->num_components++;

}

static void component_pool_remove(struct component_pool *ca, recs_entity e) {
  uint32_t component_index = ca->entity_to_comp[e];

  if(component_index == NO_COMP_ID) {
    return;
  }
  uint32_t last_component_index = ca->num_components-1;
  recs_entity entity_at_last_component = ca->comp_to_entity[last_component_index];


  //move last element to component being removed.
  //this keeps our list of components contiguous.
  //NOTE: Whether this has any performance benefits over allowing
  //our component pool buffer to be sparse is not tested.
  memcpy(
    ca->buffer + (ca->component_size * component_index),
    ca->buffer + (ca->component_size * last_component_index),
    ca->component_size
  );

  ca->comp_to_entity[component_index] = entity_at_last_component;
  ca->entity_to_comp[entity_at_last_component] = component_index;

  ca->comp_to_entity[last_component_index] = e;
  ca->entity_to_comp[e] = NO_COMP_ID;

  ca->num_components--;


}

static void component_pool_free(struct component_pool *ca) {
  //remember we made 1 big allocation starting at ca->buffer,
  //so we only need to RECS_FREE that one buffer.
  RECS_FREE(ca->buffer);
  //RECS_FREE(ca->comp_to_entity);
  //RECS_FREE(ca->entity_to_comp);
}

/*
  Component Pool End
*/



/*
  RECS Implementation Start:

  The implementation of the public headers of the RECS library.
*/


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
  uint8_t *big_buffer = RECS_MALLOC(recs_buffer_size);
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



recs_entity recs_entity_get_next_with_comps(struct recs *ecs, struct recs_comp_bitmask mask, uint32_t *index) {
  for(; (*index) < ecs->ent_man.num_active_entities; (*index)++) {
    recs_entity e = ecs->ent_man.set_of_ids[*index];
    if(recs_entity_has_components(ecs, e, mask)) {
      (*index)++;
      return e;
    }
  }
  return RECS_NO_ENTITY_ID;
}


/*
  RECS Implementation End
*/

#endif


