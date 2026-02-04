# RECS (Ryan's Entity Component System)

> Note that this library is still a work-in-progress, so expect breaking changes to be introduced in future updates until I consider this library stable.

This is a basic implementation of a sparse-set Entity-Component-System. This uses sparse sets and component pools for storing component data and mapping entity IDs to their components. 

## Design Philosophy
This ECS is designed to allocate all the memory it will ever need at the beginning of an app's lifetime.
This makes the ECS quite performant, since it has no need to allocate and free memory during gameplay.

However, this means that you cannot change the maximum number of entities, components, and systems once you initialize the ECS. For example, if you initialize this ECS to hold 1000 entities, you cannot add more than 1000 entities, nor can you reduce the maximum number of entities. Thus, make sure to properly set these values based on the requirements of your application. You can also utilize multiple ECS instances to handle different sets of component types and systems, depending on your use case.

If your application requires you to dynamically update any of the above values within the same ECS, you should use another ECS library.

## Building This Library

To build the RECS library, you will need the following dependencies:
- CMake >= 3.15
- A C compiler that supports C99 standard

Before building with CMake, you must set up your build folder using:
`cmake -S . -B build`


To build the library, you will run:
`cmake --build build --target recs`

Once you generate the library file, you will also need to 
grab the `recs.h` file within the `include` directory and include it into
your project. This header allows you to interface with the library you
just generated.


## Building The Example Code (Optional)

You will need the following dependencies installed in order to build the examples that I included:
- CMake >= 3.15
- A C compiler that supports C11 standard (some examples utilize C11 features for convenience)

Before building with CMake, you must set up your build folder using:
`cmake -S . -B build`

To build the example code for using the RECS library:
`cmake --build build --target example1`
`cmake --build build --target example2`


## Running The Tests

You will need the following dependencies installed in order to build the tests that I included:
- CMake >= 3.15
- A C compiler that supports C11 standard (some examples utilize C11 features for convenience)

Before building with CMake, you must set up your build folder using:
`cmake -S . -B build`

Here are the list of all testing targets:

To build the test code, you will need to run:
`cmake --build build --target build_tests`

To run all tests, you can use CTest to run this command:
`ctest --test-dir build --output-on-failure`


## Quick Explanation of What A Entity Component System (ECS) Is:

An Entity-Component-System (ECS) is a software architecture pattern that is primarily used in video game development to represent game world object. It is comprised of three data structures:
1. Entity
  - An entity is composed of multiple components
  - It is usually represented as a single numeric ID in memory.
2. Component
  - A component can hold any kind of data
  - It does not have any behaviors associated with them
  - Each entity can only be given 1 instance of each component type
    - If you want multiple instances of a component for a single entity, you have a few options:
      - You can create an aggregate component that stores multiple instances of the same component.
      - You can convert the original component into an aggregate component that stores multiple instances of a generic data structure with some helper functions associated with it. This avoids the need to write a system for both the single component and the aggregate component, since you now only have the aggregate component.
      - You can convert the original component into a generic data structure, then use them in more specialized components.
3. System
  - A system is a function that grabs all entities with a specific set of components and performs actions using these components, such as performing in-game updates, having entities interact with each other, and adding and removing entities to the game world.


By organizing your game objects and their behaviors this way, you can easily add and remove components at any point to any entity. Rather than writing specific code for each entity type, you write a system that interacts with all entities that share the same set of component types at once, regardless of the entity type. This modularity makes it very easy to build new entities out of already existing components and allows changes to components/systems to propagate to all entities that use that component.
 



## Features:
  - Add and remove entities
  - Attach components and tags to entities
  - Register systems and group them using an enum that you define.
  - Make a deep copy of your ECS in memory
    - Useful for games that allow you to roll back to a previous game state

  - Support for entity tags, which are essentially components with no attached data
  - Users can add custom malloc(), free(), and assert() implementations into this library
    by overwriting the RECS_MALLOC, RECS_FREE, and RECS_ASSERT macros.

  - Size Limitations
    - Maximum of 2^32 - 1 entities.
    - Maximum of 2^32 component types and tags.
    - Maximum of 2^32 - 1 component instances per component type.
    - Maximum of 2^32 systems.


## Sample Program


```c
#include <stdio.h>

// can define own malloc(), free(), and assert() macros before including ecs.h
// #define RECS_MALLOC(size) <custom malloc() here>
// #define RECS_FREE(ptr) <custom free() here>
// #define RECS_ASSERT(boolean) <custom assert() here>

#define RECS_MAX_COMPONENTS 2
#define RECS_MAX_TAGS 2
#define RECS_MAX_ENTITIES 2
#define RECS_MAX_SYSTEMS 2
#define RECS_MAX_SYS_GROUPS 1

#include "recs.h"


//define components
struct message_component {
  char message[25];
};

struct number_component {
  uint64_t num;
};


// define components, tags, and system group enums.
// note that you can define these enums manually if desired, though you will need to 
// make sure enum values start at 0 and increment by 1.

RECS_INIT_COMP_IDS(component, COMPONENT_MESSAGE, COMPONENT_NUMBER, COMPONENT_COUNT);
RECS_INIT_TAG_IDS(tag, TAG_A, TAG_B, TAG_COUNT);
RECS_INIT_SYS_GRP_IDS(system_group, SYSTEM_GROUP_UPDATE);



//define our systems
void system_print_message(struct recs *ecs) {
  uint8_t exclude_mask[RECS_GET_BITMASK_SIZE(COMPONENT_COUNT, TAG_COUNT)];
  recs_bitmask_create(ecs, exclude_mask, 0, NULL, 0, NULL);

  //iterate through EVERY entity
  recs_ent_iter iter = recs_ent_iter_init_with_exclude(ecs, NULL, exclude_mask);

  while(recs_ent_iter_has_next(&iter)) {

    //grab our entity ID
    recs_entity e = recs_ent_iter_next(ecs, &iter);

    struct message_component *m = recs_entity_get_component(ecs, e, COMPONENT_MESSAGE);
    struct number_component *n = recs_entity_get_component(ecs, e, COMPONENT_NUMBER);

    //check if our entity has a specific component, if it does, do something with it.
    if(m != NULL) {
      printf("Message: %s\n", m->message);
    }

    if(n != NULL) {
      printf("Number: %llu\n", n->num);
    }

    const char *has_tag_a_str = recs_entity_has_tag(ecs, e, TAG_A) ? "true" : "false";
    const char *has_tag_b_str = recs_entity_has_tag(ecs, e, TAG_B) ? "true" : "false";

    printf("Has Tag A = %s\n", has_tag_a_str);
    printf("Has Tag B = %s\n", has_tag_b_str);
  }
}

void system_print_number_only(struct recs *ecs) {
  //allocate enough memory to store bitmask
  uint8_t mask[RECS_GET_BITMASK_SIZE(RECS_MAX_COMPONENTS, RECS_MAX_TAGS)];

  //initialize allocated bitmask with the tags and components we want to retrieve
  recs_bitmask_create(ecs, mask,
    RECS_BITMASK_CREATE_COMP_ARG(1, COMPONENT_NUMBER), 
    RECS_BITMASK_CREATE_TAG_ARG(2, TAG_A, TAG_B)
  );

  recs_ent_iter iter = recs_ent_iter_init(ecs, mask);

  //only iterate though entities with the COMPONENT_NUMBER component and the 
  //tags TAG_A and TAG_B. 
  while(recs_ent_iter_has_next(&iter)) {
    recs_entity e = recs_ent_iter_next(ecs, &iter);
    struct number_component *n = recs_entity_get_component(ecs, RECS_ENT_ID(e), COMPONENT_NUMBER);
    printf("Entity %d with TAG_A and TAG_B has number %llu\n", RECS_ENT_ID(e), n->num);
  }
}

int main(void) {
  struct recs_init_config_component comps[RECS_MAX_COMPONENTS] = {
    {
      .type = COMPONENT_MESSAGE,
      .max_components = RECS_MAX_ENTITIES,
      .comp_size = sizeof(struct message_component)
    },
    {
      .type = COMPONENT_NUMBER,
      .max_components = RECS_MAX_ENTITIES,
      .comp_size = sizeof(struct number_component)
    }
  };

  struct recs_init_config_system systems[RECS_MAX_SYSTEMS] = {
    {
      .func = system_print_message,
      .group = SYSTEM_GROUP_UPDATE
    },
    {
      .func = system_print_number_only,
      .group = SYSTEM_GROUP_UPDATE
    }
  };

  struct recs_init_config config = {
    .max_entities = RECS_MAX_ENTITIES,
    .max_component_types = RECS_MAX_COMPONENTS,
    .max_tags = RECS_MAX_TAGS,
    .max_systems = RECS_MAX_SYSTEMS,
    .max_system_groups = RECS_MAX_SYS_GROUPS,
    .context = NULL,
    .components = comps,
    .systems = systems
  };
  //attempt to allocate and initialize our ECS
  struct recs *ecs = recs_init(config);

  //will fail if we fail to allocate enough memory for the ECS.
  if(ecs == NULL) {
    printf("Failed to initialize!\n");
    return 1;
  }


  //initialize an entity
  recs_entity e = recs_entity_add(ecs);

  //initialize and add this entity's components
  struct number_component n = {
    .num = 42
  };
  struct message_component m = {
    .message = "Hello, There"
  };

  recs_entity_add_component(ecs, e, COMPONENT_MESSAGE, &m);
  recs_entity_add_component(ecs, e, COMPONENT_NUMBER, &n);

  //assign 2 tags to this new entity
  recs_entity_add_tag(ecs, e, TAG_A);
  recs_entity_add_tag(ecs, e, TAG_B);
  

  //run all registered systems tagged with RECS_SYSTEM_TYPE_UPDATE
  //in the order they are registered in.
  recs_system_run(ecs, SYSTEM_GROUP_UPDATE);

  //queue 1st entity to be removed. Note that doing this also disables the entity,
  //which prevents it from being returned from iterators
  recs_entity_queue_remove(ecs, e);
  recs_system_run(ecs, SYSTEM_GROUP_UPDATE);


  //remove the queued entity from the active entity list
  recs_entity_remove_queued(ecs);


  //ecs needs to be freed once it is no longer needed.
  recs_free(ecs);
  return 0;
}

```

## Potential Upcoming Features
- Allow more efficient way to query entities within systems based on their components and tags.
  - Perhaps users could tell the ECS what types of queries they want to make on initialization of ECS, that way each entity gets stored within a specific array containing entities with the same set of elements.
- Allow ECS to be serialized and deserialized. This allows you to save ECS state to a file and restore the ECS from a file.


## External Resources About ECS and Other ECS Projects
- https://en.wikipedia.org/wiki/Entity_component_system
- https://github.com/SanderMertens/ecs-faq
- https://github.com/SanderMertens/flecs